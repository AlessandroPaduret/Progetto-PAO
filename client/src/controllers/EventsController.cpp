#include "controllers/EventsController.h"

#include "api/ApiClient.h"

namespace client {

EventsController::EventsController(ApiClient* api, QObject* parent)
    : QObject(parent), m_api(api) {}

QDateTime EventsController::from() const {
    return m_from;
}

QDateTime EventsController::to() const {
    return m_to;
}

void EventsController::setRange(const QDateTime& from, const QDateTime& to) {
    m_from = from;
    m_to = to;
    refresh();
}

void EventsController::refresh() {
    connect(m_api, &ApiClient::eventsLoaded, this,
            &EventsController::eventsChanged, Qt::SingleShotConnection);
    connect(m_api, &ApiClient::requestFailed, this,
            &EventsController::errorOccurred, Qt::SingleShotConnection);

    m_api->getEvents(m_from, m_to);
}

void EventsController::wireOperation() {
    connect(m_api, &ApiClient::operationSucceeded, this,
            &EventsController::noticeOccurred, Qt::SingleShotConnection);
    connect(m_api, &ApiClient::operationSucceeded, this,
            &EventsController::refresh, Qt::SingleShotConnection);
    connect(m_api, &ApiClient::requestFailed, this,
            &EventsController::errorOccurred, Qt::SingleShotConnection);
}

void EventsController::createEvent(const CreateEventRequest& request) {
    wireOperation();
    m_api->createEvent(request);
}

void EventsController::deleteEvent(qint64 eventId) {
    wireOperation();
    m_api->deleteEvent(eventId);
}

void EventsController::addException(qint64 eventId, const QDateTime& exception) {
    wireOperation();
    m_api->addException(eventId, exception);
}

} // namespace client
