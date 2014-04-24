#ifndef SESSIONITEM_H
#define SESSIONITEM_H

#include "GrooveOffNamespace.h"

#include <QWidget>

namespace Ui {
class SessionItem;
}

class SessionItem : public QWidget
{
    Q_OBJECT

public:
    explicit SessionItem(const QString &name, int num, QWidget *parent = 0);
    ~SessionItem();
    QString sessionName() const;
    void updateNumTrack(int);

    void deleteButtonEnabled(bool ok);

protected:
    virtual void leaveEvent ( QEvent * event );
    virtual void enterEvent ( QEvent * event );

Q_SIGNALS:
    void remove(const QString &);

private Q_SLOTS:
    void multiFuncBtnClicked();
    void removeSession();

private:
    Ui::SessionItem *ui;
    GrooveOff::DownloadState state_;
    bool deleteButtonEnabled_;

    void setupUi();
    void setupConnections();
    void stateChanged();
};

#endif // SESSIONITEM_H
