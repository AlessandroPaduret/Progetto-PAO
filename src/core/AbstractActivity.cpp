/*		IMPLEMENTATION FOR ABSTRACTACTIVITY		*/

#include 	<core/AbstractActivity.h>
#include	<QUuid>

// Member initialization
AbstractActivity::AbstractActivity(const QString& title, QString id) :
	m_id(id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : std::move(id)),
	m_title(title),
	m_flags(static_cast<std::uint8_t>(State::None)) {}
