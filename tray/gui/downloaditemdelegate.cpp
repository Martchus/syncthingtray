#include "./downloaditemdelegate.h"

#include "../../model/syncthingdownloadmodel.h"

#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QTextOption>
#include <QStyleOptionViewItem>
#include <QBrush>
#include <QPalette>
#include <QFontMetrics>

#include <iostream>

using namespace std;
using namespace Data;

namespace QtGui {

inline int centerObj(int avail, int size)
{
    return (avail - size) / 2;
}

DownloadItemDelegate::DownloadItemDelegate(QObject* parent) :
    QStyledItemDelegate(parent),
    m_folderIcon(QIcon::fromTheme(QStringLiteral("folder-open"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder-open.svg"))).pixmap(QSize(16, 16)))
{}

void DownloadItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // init style options to use drawControl(), except for the text
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.textElideMode = Qt::ElideNone; // elide manually
    opt.features = QStyleOptionViewItem::None;
    if(index.parent().isValid()) {
        opt.displayAlignment = Qt::AlignTop | Qt::AlignLeft;
        opt.decorationSize = QSize(option.rect.height(), option.rect.height());
        opt.features |= QStyleOptionViewItem::HasDecoration;
        opt.text = option.fontMetrics.elidedText(opt.text, Qt::ElideMiddle, opt.rect.width() - opt.rect.height() - 26);
    } else {
        opt.text = option.fontMetrics.elidedText(opt.text, Qt::ElideMiddle, opt.rect.width() / 2 - 4);
    }
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

    // draw progress bar
    const QAbstractItemModel *model = index.model();
    QStyleOptionProgressBar progressBarOption;
    progressBarOption.state = option.state;
    progressBarOption.direction = option.direction;
    progressBarOption.rect = option.rect;
    if(index.parent().isValid()) {
        progressBarOption.rect.setX(opt.rect.x() + opt.rect.height() + 4);
        progressBarOption.rect.setY(opt.rect.y() + opt.rect.height() / 2);
    } else {
        progressBarOption.rect.setX(opt.rect.x() + opt.fontMetrics.width(opt.text) + 6);
        progressBarOption.rect.setWidth(progressBarOption.rect.width() - 18);
    }
    progressBarOption.textAlignment = Qt::AlignCenter;
    progressBarOption.textVisible = true;
    if(option.state & QStyle::State_Selected) {
        progressBarOption.palette.setBrush(QPalette::Foreground, option.palette.brush(QPalette::HighlightedText));
    }
    progressBarOption.progress = model->data(index, SyncthingDownloadModel::ItemPercentage).toInt();
    progressBarOption.minimum = 0;
    progressBarOption.maximum = 100;
    progressBarOption.text = model->data(index, SyncthingDownloadModel::ItemProgressLabel).toString();
    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);

    // draw buttons
    int buttonY = option.rect.y();
    if(!index.parent().isValid()) {
        buttonY += centerObj(progressBarOption.rect.height(), 16);
    }
    painter->drawPixmap(option.rect.right() - 16, buttonY, 16, 16, m_folderIcon);

    // draw file icon
    if(index.parent().isValid()) {
        const int fileIconHeight = option.rect.height() - 2;
        painter->drawPixmap(option.rect.left(), option.rect.y() + 1, fileIconHeight, fileIconHeight, model->data(index, Qt::DecorationRole).value<QIcon>().pixmap(fileIconHeight));
    }
}

QSize DownloadItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize defaultSize(QStyledItemDelegate::sizeHint(option, index));
    if(index.parent().isValid()) {
        defaultSize.setHeight(defaultSize.height() + defaultSize.height() - 12);
    }
    return defaultSize;
}

}
