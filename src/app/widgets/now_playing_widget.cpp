#include "app/widgets/now_playing_widget.hpp"

#include <QVariant>
#include <QStyle>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QFontMetrics>

NowPlayingWidget::NowPlayingWidget(QWidget *parent)
    : QWidget(parent)
{
    this->setObjectName("NowPlaying");

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 4, 10, 4);
    layout->setSpacing(10);

    tag_ = new QLabel("—", this);
    tag_->setObjectName("NowPlayingTag");
    tag_->setMinimumWidth(42);
    tag_->setAlignment(Qt::AlignCenter);

    text_ = new QLabel("No media", this);
    text_->setObjectName("NowPlayingText");
    text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    text_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    layout->addWidget(tag_);
    layout->addWidget(text_, 1);

    marquee_ = new QTimer(this);
    marquee_->setInterval(110); // smooth enough, cheap
    connect(marquee_, &QTimer::timeout, this, [this]{ refreshMarquee(); });
    marquee_->start();

    setActive(false);
}

void NowPlayingWidget::setSourceTag(const QString &tag)
{
    tag_->setText(tag);
}

void NowPlayingWidget::setNowPlaying(const QString &text)
{
    full_ = text.trimmed();
    offset_ = 0;

    if (full_.isEmpty()) {
        text_->setText("No media");
        setActive(false);
        return;
    }

    setActive(true);
    text_->setText(full_);
}

void NowPlayingWidget::setActive(bool active)
{
    // Force QVariant for Qt5
    this->setProperty("active", QVariant(active));

    // Re-apply style so QSS attribute selectors update
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
}

void NowPlayingWidget::refreshMarquee()
{
    if (full_.isEmpty())
        return;

    // If it fits, don't scroll.
    QFontMetrics fm(text_->font());
    const int avail = text_->width();
    const int fullW = fm.horizontalAdvance(full_);
    if (fullW <= avail) {
        if (text_->text() != full_) text_->setText(full_);
        return;
    }

    // Build a looping string with spacing
    const QString loop = full_ + "   •   " + full_;
    const int loopW = fm.horizontalAdvance(loop);

    // Convert offset in pixels to a substring-ish by walking characters
    // (cheap, good enough for UI)
    int x = offset_;
    while (x > loopW) x -= loopW;
    offset_ += 8; // scroll speed

    // Render a sliding window by trimming leading chars until width matches
    QString out = loop;
    while (fm.horizontalAdvance(out) > avail + 80) { // keep some buffer
        out.remove(0, 1);
    }
    text_->setText(out);
}
