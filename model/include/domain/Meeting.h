#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <cstddef>

#include "core/Activity.h"
#include "core/CommonTypes.h"
#include "core/DateGenerator.h"

namespace events {

/** @brief Riunione: sotto-classe di Activity con luogo e partecipanti. */
class Meeting : public Activity {
private:
    String m_location;
    std::vector<String> m_attendees;  ///< senza duplicati

public:
    /** @brief Costruttore della riunione. @param title,start Obbligatori. */
    explicit Meeting(String title,
                     TimePoint start,
                     Duration duration = Duration::zero(),
                     String location = "",
                     std::shared_ptr<const DateGenerator> generator = nullptr,
                     TimePoint end = TimePoint::max());

    ~Meeting() override = default;

    String getLocation() const { return m_location; }
    void setLocation(const String& location) { m_location = location; }

    std::size_t attendeeCount() const { return m_attendees.size(); }
    const std::vector<String>& getAttendees() const { return m_attendees; }

    /** @brief Aggiunge un partecipante. @return false se gia' presente (niente duplicati). */
    bool addAttendee(const String& attendee);
    bool removeAttendee(const String& attendee);

    String describe() const override;
    void accept(ActivityVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<Activity> clone() const override;
};

} // namespace events