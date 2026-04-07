#include <ScrollingNotification.h>

#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>

using namespace GUI;

ScrollingNotification::ScrollingNotification(QWidget* parent)
   : QWidget(parent), offset(0), visible(false)
{
   setFixedHeight(20);
   hide();
   connect(&this->scrollTimer, &QTimer::timeout, this, &ScrollingNotification::scroll);
}

void ScrollingNotification::showMessage(const QString& text)
{
   this->message = text;
   this->offset = this->width();
   this->visible = true;
   show();
   this->scrollTimer.start(30);
}

void ScrollingNotification::dismiss()
{
   this->scrollTimer.stop();
   this->visible = false;
   hide();
}

void ScrollingNotification::paintEvent(QPaintEvent*)
{
   if (!this->visible)
      return;

   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing);

   painter.fillRect(rect(), QColor(40, 120, 200));
   painter.setPen(Qt::white);
   painter.setFont(font());
   painter.drawText(this->offset, height() / 2 + QFontMetrics(font()).ascent() / 2, this->message);
}

void ScrollingNotification::mousePressEvent(QMouseEvent* event)
{
   if (event->button() == Qt::LeftButton)
      emit clicked();
}

void ScrollingNotification::scroll()
{
   this->offset -= 2;
   const int textWidth = QFontMetrics(font()).horizontalAdvance(this->message);
   if (this->offset < -textWidth)
      this->offset = this->width();
   update();
}
