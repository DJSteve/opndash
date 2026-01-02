#pragma once

class QWidget;
class Dash;

namespace Ui {
    // Builds the whole control bar widget (buttons + layout + styling hooks)
    QWidget *build_control_bar(Dash *dash);
}
