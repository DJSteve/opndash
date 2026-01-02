#include "app/window.hpp"

#include <QSizePolicy>

#include "app/utilities/icon_engine.hpp"
#include "app/widgets/pulse_vu_meter.hpp"
#include "app/widgets/rail_volume.hpp"
#include "app/widgets/nav_neon_indicator.hpp"


void Dash::build_nav_rail_and_pages()
{
    // Build sidebar page buttons
    for (auto page : this->arbiter.layout().pages()) {
        auto button = page->button();
        button->setCheckable(true);
        button->setFlat(true);

        // consistent button sizing for high-DPI
        button->setMinimumSize(56, 56);
        button->setMaximumSize(56, 56);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QIcon icon(new StylizedIconEngine(
            this->arbiter,
            QString(":/icons/%1.svg").arg(page->icon_name()),
            true
        ));
        this->arbiter.forge().iconize(icon, button, 32);

        this->rail.group.addButton(button, this->arbiter.layout().page_id(page));
        this->rail.layout->addWidget(button);
        this->body.frame->addWidget(page->container());

        page->init();
        button->setVisible(page->enabled());
    }

    // ---- VU meter (below page buttons) ----
    auto vu = new PulseVUMeter(this->rail.widget);
    vu->setFixedWidth(64);
    vu->setFixedHeight(540);
    this->rail.layout->addWidget(vu, 0, Qt::AlignHCenter);
    this->rail.layout->addSpacing(10);

    // push volume widget to the bottom
    this->rail.layout->addStretch(1);

    auto railVol = new RailVolume(this->arbiter, this->rail.widget);
    this->rail.layout->addWidget(railVol, 0, Qt::AlignHCenter);

    // set initial page
    this->set_page(this->arbiter.layout().curr_page);

    // Snap neon indicator to current page on boot (no animation)
    if (this->nav_neon) {
        int id = this->arbiter.layout().page_id(this->arbiter.layout().curr_page);
        if (auto b = this->rail.group.button(id))
            this->nav_neon->moveToButton(b, false);
    }
}
