#include "ImgDiffReportDlg.h"
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QTextOption>
#include <QVBoxLayout>

namespace
{
struct DiffMetric
{
    QString component;
    QString maxDiff;
    QString maxDiffPosition;
    QString minDiff;
    QString meanDiff;
};

QString colorName(const QColor &color)
{
    return color.name(QColor::HexRgb);
}
}

ImgDiffReportDlg::ImgDiffReportDlg(QWidget *parent): QDialog(parent), reportView(nullptr), gotoMaxDiffBtn(nullptr)
{
    initUI();
}

void ImgDiffReportDlg::initUI()
{
    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(16, 16, 16, 16);
    vlayout->setSpacing(12);
    reportView = new QTextBrowser(this);
    reportView->setOpenLinks(false);
    reportView->setFrameShape(QFrame::NoFrame);
    reportView->setStyleSheet(QStringLiteral("QTextBrowser { background: palette(base); }"));
    QTextOption textOption = reportView->document()->defaultTextOption();
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    reportView->document()->setDefaultTextOption(textOption);
    gotoMaxDiffBtn = new QPushButton(tr("jump to max diff"), this);
    connect(gotoMaxDiffBtn, &QPushButton::clicked, this, &ImgDiffReportDlg::onGotoMaxDiffClicked);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    btnLayout->addWidget(gotoMaxDiffBtn);

    vlayout->addWidget(reportView, 1);
    vlayout->addLayout(btnLayout);

    setMinimumSize(720, 420);
    resize(860, 500); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    setWindowTitle(tr("img difference report"));
}

void ImgDiffReportDlg::setReportInfo(const QString& info)
{
    const QStringList lines = info.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QString comparisonSeparator = QStringLiteral(" <------> ");
    const int separatorIndex = lines.isEmpty() ? -1 : lines.first().indexOf(comparisonSeparator);
    if (separatorIndex < 0)
    {
        reportView->setPlainText(info);
        return;
    }

    const QString leftPath = lines.first().left(separatorIndex);
    const QString rightPath = lines.first().mid(separatorIndex + comparisonSeparator.size());
    const QRegularExpression metricPattern(
        QStringLiteral("^(?:(Y|U|V) )?max diff:(\\S+) @ \\[(\\d+), (\\d+)\\], min diff:(\\S+), mean diff:(\\S+)\\s*$"));
    QList<DiffMetric> metrics;
    for (int i = 1; i < lines.size(); ++i)
    {
        const QRegularExpressionMatch match = metricPattern.match(lines.at(i).trimmed());
        if (!match.hasMatch())
        {
            reportView->setPlainText(info);
            return;
        }
        metrics.append({
            match.captured(1).isEmpty() ? tr("Pixel") : match.captured(1),
            match.captured(2),
            QStringLiteral("[%1, %2]").arg(match.captured(3), match.captured(4)),
            match.captured(5),
            match.captured(6)
        });
    }
    if (metrics.isEmpty())
    {
        reportView->setPlainText(info);
        return;
    }

    const QPalette colors = reportView->palette();
    const QString textColor = colorName(colors.color(QPalette::Text));
    const QString mutedColor = colorName(colors.color(QPalette::Mid));
    const QString borderColor = colorName(colors.color(QPalette::Midlight));
    const QString accentColor = colorName(colors.color(QPalette::Highlight));
    QString html = QStringLiteral(
        "<html><body style='font-family: sans-serif; color:%1;'>"
        "<h2 style='margin: 0 0 12px 0;'>%2</h2>"
        "<table width='100%%' cellspacing='8' cellpadding='12'>"
        "<tr><td bgcolor='%3' style='border:1px solid %4;'>"
        "<span style='color:%5; word-wrap:break-word;'>%6</span></td>"
        "<td bgcolor='%3' style='border:1px solid %4;'>"
        "<span style='color:%5; word-wrap:break-word;'>%7</span></td></tr>"
        "</table>"
        "<h3 style='margin:16px 0 8px 0;'>%8</h3>"
        "<table width='100%%' cellspacing='0' cellpadding='8' style='border-collapse:collapse;'>"
        "<tr bgcolor='%3'>"
        "<th align='left' style='border:1px solid %4;'>%9</th>"
        "<th align='right' style='border:1px solid %4;'>%10</th>"
        "<th align='right' style='border:1px solid %4;'>%11</th>"
        "<th align='right' style='border:1px solid %4;'>%12</th>"
        "<th align='right' style='border:1px solid %4;'>%13</th></tr>")
        .arg(textColor,
             tr("Image comparison").toHtmlEscaped(),
             colorName(colors.color(QPalette::AlternateBase)),
             borderColor,
             mutedColor,
             leftPath.toHtmlEscaped(),
             rightPath.toHtmlEscaped(),
             tr("Difference metrics").toHtmlEscaped(),
             tr("Component").toHtmlEscaped(),
             tr("Max diff").toHtmlEscaped(),
             tr("Position (row, column)").toHtmlEscaped(),
             tr("Min diff").toHtmlEscaped(),
             tr("Mean diff").toHtmlEscaped());

    for (int i = 0; i < metrics.size(); ++i)
    {
        const DiffMetric &metric = metrics.at(i);
        const QString rowColor = i % 2 == 0 ? colorName(colors.color(QPalette::Base)) : colorName(colors.color(QPalette::AlternateBase));
        html += QStringLiteral(
            "<tr bgcolor='%1'>"
            "<td style='border:1px solid %2;'><b>%3</b></td>"
            "<td align='right' style='border:1px solid %2; color:%4;'><b>%5</b></td>"
            "<td align='right' style='border:1px solid %2;'>%6</td>"
            "<td align='right' style='border:1px solid %2;'>%7</td>"
            "<td align='right' style='border:1px solid %2;'>%8</td></tr>")
            .arg(rowColor, borderColor, metric.component.toHtmlEscaped(), accentColor,
                 metric.maxDiff.toHtmlEscaped(), metric.maxDiffPosition.toHtmlEscaped(),
                 metric.minDiff.toHtmlEscaped(), metric.meanDiff.toHtmlEscaped());
    }
    html += QStringLiteral("</table></body></html>");
    reportView->setHtml(html);
}

void ImgDiffReportDlg::onGotoMaxDiffClicked()
{
    done(GOTO_MAX_DIFF_RESULT);
}
