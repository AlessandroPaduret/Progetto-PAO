#pragma once

#include <QStringList>
#include <QWidget>

class QLineEdit;
class QListWidget;

namespace app {

/** @brief Sezione specifica della Riunione (luogo + partecipanti), estratta
 *  da ActivitySidebarWidget in un widget figlio autonomo: non conosce
 *  `events::Meeting`, si limita a raccogliere luogo e nomi in una QStringList
 *  che il genitore traduce da/verso il modello. */
class MeetingFormWidget : public QWidget {
    Q_OBJECT
public:
    explicit MeetingFormWidget(QWidget* parent = nullptr);

    QString location() const;
    QStringList attendees() const;

    void setLocation(const QString& location);
    void setAttendees(const QStringList& attendees);

    /** @brief Svuota luogo e partecipanti (nuova attivita'). */
    void clear();

private slots:
    void onAddAttendee();
    void onRemoveAttendee();

private:
    QLineEdit* m_locationEdit = nullptr;
    QLineEdit* m_attendeeEdit = nullptr;
    QListWidget* m_attendeesList = nullptr;
};

} // namespace app
