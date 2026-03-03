#ifndef READINGSESSION_H
#define READINGSESSION_H

#include	"../../core/AbstractActivity.h"
#include 	"../../core/AbstractVisitor.h"
#include 	<QString>


class ReadingSession : public AbstractActivity {
public:
	// Ctor
	ReadingSession(const QString& title, const QString& pdfPath);

	// --- KERNEL IMPLEMENTATION CONTRACT ---
	// Visitor override magic
	void accept(ActivityVisitor& visitor) override;

	// Basic Json for test
	// QJsonObject toJson() const override;

	// Cloning prototype
	std::unique_ptr<AbstractActivity> clone() const override;

	// --- SPECIFIC CONCRETE METHODS FOR RS ---
	const QString& getPdfPath() const { return m_pdfPath; }

private:
	QString m_pdfPath;
	// std::vector<Note> will prolly go here
};

#endif // READINGSESSION_H
