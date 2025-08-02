#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include <QtWidgets/QAbstractButton>


class QPropertyAnimation;

class ToggleButton :public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(int mOffset READ offset WRITE setOffset);
public:
    explicit ToggleButton(QWidget* parent = nullptr);
    explicit ToggleButton(int trackRadius, int thumbRadius, QWidget* parent = nullptr);
    ~ToggleButton();

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent*) override;
    void mouseReleaseEvent(QMouseEvent  *) override;
    void enterEvent(QEnterEvent *event) override;
    void setChecked(bool checked);

    int offset();
    void setOffset(int value);

private:
    qreal mOffset;
    qreal mBaseOffset;
    qreal mMargin;
    qreal mTrackRadius;
    qreal mThumbRadius;
    qreal mOpacity;
    QPropertyAnimation* mAnimation;

    QHash<bool, qreal> mEndOffset;
    QHash<bool, QBrush> mTrackColor;
    QHash<bool, QBrush> mThumbColor;
    QHash<bool, QColor> mTextColor;
    QHash<bool, QString> mThumbText;

    void updateOffsetRange();  // ← 新增

};

#endif // TOGGLEBUTTON_H
