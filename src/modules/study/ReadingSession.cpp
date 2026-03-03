#include	"../../../include/modules/study/ReadingSession.h"
#include 	<QJsonObject>
#include 	<memory>

ReadingSession::ReadingSession(const QString& title, const QString& pdfPath)
	: AbstractActivity(title),
	m_pdfPath(pdfPath) {}

// Visitor instructions
void ReadingSession::accept(ActivityVisitor& visitor) {
	// at this point, *this is ReadingSession& by context
	// the compiler then, will choose visit(ReadingSession ...) from the visitor
	visitor.visit(*this);
}

std::unique_ptr<AbstractActivity> ReadingSession::clone() const {
	// Copy ctor for making a new clean instance
	auto copy = std::make_unique<ReadingSession>(getTitle(), m_pdfPath);
	// Note: the id will be different, but if the copy was identic byte per byte, then
	// the id should be passed to the ctor, it is assumed that the new activity is "id new" one
	return copy;
}
