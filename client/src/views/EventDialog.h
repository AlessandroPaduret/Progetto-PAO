#ifndef CLIENT_EVENTDIALOG_H
#define CLIENT_EVENTDIALOG_H

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QSpinBox;

namespace client {

struct CreateEventRequest;
struct Occurrence;

/** @brief Dialog per la creazione di un evento (semplice o ricorrente). */
class EventDialog : public QDialog {
    Q_OBJECT
public:
    explicit EventDialog(QWidget* parent = nullptr);

    /** @brief Preimposta l'orario di inizio (usato dal doppio clic su una cella). */
    void setStart(const QDateTime& start);

    /** @brief Preimposta i campi da un'occorrenza esistente e forza "singolo"
     *  (usato per la modifica di una singola istanza).
     */
    void setOccurrence(const Occurrence& occurrence);

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
};

} // namespace client

#endif // CLIENT_EVENTDIALOG_H
