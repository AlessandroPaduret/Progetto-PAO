#ifndef CLIENT_EVENTDIALOG_H
#define CLIENT_EVENTDIALOG_H

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;

namespace client {

struct CreateEventRequest;

/** @brief Dialog per la creazione di un evento (semplice o ricorrente). */
class EventDialog : public QDialog {
    Q_OBJECT
public:
    explicit EventDialog(QWidget* parent = nullptr);

    /** @brief Costruisce la richiesta dai campi compilati. */
    CreateEventRequest request() const;

private slots:
    void onTypeChanged();
    void onAccept();

private:
    QLineEdit* m_title;
    QDateTimeEdit* m_start;
    QSpinBox* m_durationMinutes;
    QComboBox* m_type;
    QSpinBox* m_intervalDays;
    QCheckBox* m_hasEnd;
    QDateTimeEdit* m_end;
    QListWidget* m_exceptions;
    QDateTimeEdit* m_exceptionPicker;
};

} // namespace client

#endif // CLIENT_EVENTDIALOG_H
