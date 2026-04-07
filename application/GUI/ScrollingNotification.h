#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QString>

namespace GUI
{
   class ScrollingNotification : public QWidget
   {
      Q_OBJECT
   public:
      explicit ScrollingNotification(QWidget* parent = nullptr);

   public slots:
      void showMessage(const QString& text);
      void dismiss();

   signals:
      void clicked();

   protected:
      void paintEvent(QPaintEvent* event) override;
      void mousePressEvent(QMouseEvent* event) override;

   private slots:
      void scroll();

   private:
      QString message;
      int offset;
      QTimer scrollTimer;
      bool visible;
   };
}
