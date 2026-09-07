#pragma once

#include <QStringList>

#include <memory>

#include "dialog/ActivityTypeWidget.h"

class QLineEdit;
class QListWidget;

namespace app {

/** @brief Sezione specifica della Riunione (luogo + partecipanti):
 *  implementa ActivityTypeWidget, sa da sola svuotarsi/popolarsi/costruire
 *  il proprio events::Meeting. */
class MeetingFormWidget : public ActivityTypeWidget {
    Q_OBJECT
public:
    explicit MeetingFormWidget(QWidget* parent = nullptr);

    QString location() const;
    QStringList attendees() const;

    void setLocation(const QString& location);
    void setAttendees(const QStringList& attendees);

    void clear() override;
    void populateFrom(const events::Activity& activity) override;
    void applyToConfig(events::ActivityConfig& config) const override;
    std::unique_ptr<events::Activity> createActivity(events::ActivityConfig config) const override;

private slots:
    void onAddAttendee();
    void onRemoveAttendee();

private:
    QLineEdit* m_locationEdit = nullptr;
    QLineEdit* m_attendeeEdit = nullptr;
    QListWidget* m_attendeesList = nullptr;
};

} // namespace app
