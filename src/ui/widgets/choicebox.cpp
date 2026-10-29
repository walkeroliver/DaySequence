// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "choicebox.h"
#include <QtWidgets>

class ChoicePopup final : public QFrame {
  public:
    explicit ChoicePopup(QWidget *parent) : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint) {}
    Theme theme = Theme::named("forest");

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(theme.line, 1));
        painter.setBrush(theme.panel);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
    }
};
class ChoiceDelegate final : public QStyledItemDelegate {
  public:
    explicit ChoiceDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    Theme theme = Theme::named("forest");
    int selected = -1;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return {160, 42};
    }
    void paint(QPainter *p, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRect r = option.rect.adjusted(3, 2, -3, -2);
        const bool checked = index.row() == selected,
                   hover = option.state & (QStyle::State_MouseOver | QStyle::State_Selected);
        p->setPen(Qt::NoPen);
        p->setBrush(checked || hover ? theme.soft : theme.panel);
        p->drawRoundedRect(r, 7, 7);
        int left = r.left() + 12;
        auto icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!icon.isNull()) {
            const int w = qMax(18, icon.actualSize(QSize(64, 18)).width());
            icon.paint(p, QRect(left, r.center().y() - 9, w, 18));
            left += w + 10;
        }
        QFont font = option.font;
        font.setPixelSize(13);
        font.setBold(checked);
        p->setFont(font);
        p->setPen(index.flags().testFlag(Qt::ItemIsEnabled) ? (checked ? theme.accent : theme.text)
                                                            : theme.muted);
        p->drawText(QRect(left, r.top(), qMax(1, r.right() - left - 30), r.height()),
                    Qt::AlignVCenter | Qt::AlignLeft,
                    QFontMetrics(font).elidedText(index.data().toString(), Qt::ElideRight,
                                                  qMax(1, r.right() - left - 30)));
        if (checked) {
            p->setPen(QPen(theme.accent, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const int x = r.right() - 19, y = r.center().y();
            p->drawLine(x - 4, y, x - 1, y + 3);
            p->drawLine(x - 1, y + 3, x + 5, y - 4);
        }
        p->restore();
    }
};
ChoiceBox::ChoiceBox(QWidget *parent)
    : QComboBox(parent), popup(new ChoicePopup(this)), list(new QListView(popup)),
      delegate(new ChoiceDelegate(list)) {
    setMinimumHeight(36);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    popup->setObjectName("choicePopup");
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->installEventFilter(this);
    auto layout = new QVBoxLayout(popup);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(0);
    layout->addWidget(list);
    list->setObjectName("choiceList");
    list->setItemDelegate(delegate);
    list->setMouseTracking(true);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->installEventFilter(this);
    connect(list, &QListView::clicked, this, &ChoiceBox::activate);
    setTheme(Theme::named(qApp->property("daysequenceTheme").toString()));
}
void ChoiceBox::setTheme(const Theme &value) {
    theme = value;
    static_cast<ChoicePopup *>(popup)->theme = value;
    popup->update();
    delegate->theme = value;
    popup->setStyleSheet(
        QString("QFrame#choicePopup{background:%1;border:1px solid %2;border-radius:10px;} "
                "QListView#choiceList{background:transparent;border:0;outline:0;} "
                "QListView#choiceList::item{border:0;}")
            .arg(theme.panel.name(), theme.line.name()));
    list->viewport()->update();
    update();
}
QSize ChoiceBox::sizeHint() const {
    int width = 100;
    const QFontMetrics metrics(font());
    for (int i = 0; i < count(); ++i)
        width = qMax(width, metrics.horizontalAdvance(itemText(i)) + 46 +
                                (itemIcon(i).isNull() ? 0 : iconSize().width() + 10));
    return {width, 36};
}
void ChoiceBox::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = rect().adjusted(0, 0, -1, -1);
    const bool active = hasFocus() || popup->isVisible();
    p.setPen(QPen(active ? theme.accent : theme.line, active ? 1.3 : 1));
    p.setBrush(theme.panel);
    p.drawRoundedRect(r, 8, 8);
    int left = 12;
    const auto icon = currentIndex() >= 0 ? itemIcon(currentIndex()) : QIcon();
    if (!icon.isNull()) {
        icon.paint(&p, QRect(left, (height() - iconSize().height()) / 2, iconSize().width(),
                             iconSize().height()));
        left += iconSize().width() + 9;
    }
    p.setFont(font());
    p.setPen(isEnabled() ? theme.text : theme.muted);
    p.drawText(
        QRect(left, 0, qMax(1, width() - left - 30), height()), Qt::AlignVCenter | Qt::AlignLeft,
        fontMetrics().elidedText(currentText(), Qt::ElideRight, qMax(1, width() - left - 30)));
    p.setPen(QPen(isEnabled() ? theme.muted : theme.line, 1.4, Qt::SolidLine, Qt::RoundCap,
                  Qt::RoundJoin));
    const qreal x = width() - 17, y = height() / 2.0;
    p.drawLine(QPointF(x - 4, y - 2), QPointF(x, y + 2));
    p.drawLine(QPointF(x, y + 2), QPointF(x + 4, y - 2));
}
void ChoiceBox::showPopup() {
    if (!isEnabled() || count() == 0)
        return;
    list->setModel(model());
    list->setModelColumn(modelColumn());
    list->setRootIndex(rootModelIndex());
    delegate->selected = currentIndex();
    const auto available = screen()->availableGeometry();
    const auto below = mapToGlobal(QPoint(0, height() + 5));
    int w = qMin(qMax(190, qMax(width(), sizeHint().width() + 24)), available.width() - 16);
    int h = qMin(count() * 42 + 14, 350);
    int y = below.y();
    const int spaceBelow = available.bottom() - y - 7,
              spaceAbove = mapToGlobal(QPoint()).y() - available.top() - 12;
    if (h > spaceBelow && spaceAbove > spaceBelow) {
        h = qMin(h, spaceAbove);
        y = mapToGlobal(QPoint()).y() - h - 5;
    } else
        h = qMin(h, spaceBelow);
    h = qMax(54, h);
    const int x = qBound(available.left() + 8, below.x(), available.right() - w - 7);
    y = qBound(available.top() + 8, y, available.bottom() - h - 7);
    popup->setGeometry(x, y, w, h);
    list->setCurrentIndex(model()->index(currentIndex(), modelColumn(), rootModelIndex()));
    popup->show();
    list->setFocus();
    list->scrollTo(list->currentIndex());
    update();
}
void ChoiceBox::hidePopup() {
    popup->hide();
    QComboBox::hidePopup();
    update();
}
void ChoiceBox::activate(const QModelIndex &index) {
    if (!index.isValid() || !index.flags().testFlag(Qt::ItemIsEnabled))
        return;
    setCurrentIndex(index.row());
    hidePopup();
    setFocus(Qt::PopupFocusReason);
    emit activated(index.row());
    emit textActivated(currentText());
}
void ChoiceBox::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter || event->key() == Qt::Key_Down ||
        event->key() == Qt::Key_Up) {
        showPopup();
        event->accept();
        return;
    }
    QComboBox::keyPressEvent(event);
}
void ChoiceBox::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (popup->isVisible())
            hidePopup();
        else
            showPopup();
        event->accept();
        return;
    }
    QComboBox::mousePressEvent(event);
}
void ChoiceBox::wheelEvent(QWheelEvent *event) {
    event->ignore();
}
bool ChoiceBox::eventFilter(QObject *object, QEvent *event) {
    if (object == list && event->type() == QEvent::KeyPress) {
        auto key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape) {
            hidePopup();
            setFocus(Qt::PopupFocusReason);
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter ||
            key->key() == Qt::Key_Space) {
            activate(list->currentIndex());
            return true;
        }
    }
    if (object == popup && event->type() == QEvent::Hide) {
        QComboBox::hidePopup();
        update();
    }
    return QComboBox::eventFilter(object, event);
}
