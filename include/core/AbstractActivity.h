#ifndef ABSTRACTACTIVITY_H
#define ABSTRACTACTIVITY_H

// Note: Qute has it's own primitives optimized, so we gon use them
#include	<QString>
#include	<QJsonObject>
#include	<cstdint> // For uint64_t

// Forward dec for Visitor (GUI separation)
class ActivityVisitor;

class AbstractActivity {
public:
	enum class State : std::uint8_t {
		None		= 0,
		Completed	= 1 << 0, // Useful for ale(event completed)
		Archived	= 1 << 1, // Maybe common (?)..whatever flags are not important rn
		Urgent		= 1 << 2,
		Locked		= 1 << 3, // MIINE (file is being used)
		Exported	= 1 << 4  // Mine (for anki!)
	};

	// Comments done with good syntax must be implemented
	explicit AbstractActivity(const QString& title, QString id = QString());

	// Dctr
	virtual ~AbstractActivity() noexcept = default;

	// -- POLIMORPHIC CONTRACT(Pure) --
	virtual void accept(ActivityVisitor& visitor) = 0;
	// virtual QJsonObject toJson() const = 0; MUST BE DISCUSSED, mi son dimenticato cosa avevamo detto riguardo questo Ale
	virtual std::unique_ptr<AbstractActivity> clone() const = 0; // Modern C++ prototype pattern
	
	// -- ACCESS DATA AND STATE (Non virtual and inlined for performace overhead)
	const QString& getId() const noexcept { return m_id; }
	const QString& getTitle() const noexcept { return m_title; }
	void setTitle(const QString& title) { m_title = title; }

	// Bitwise manipulation for state info
	bool hasState(State mask) const noexcept { return (m_flags & static_cast<std::uint8_t>(mask)); }
	// ...
	
private:
	QString m_id;		// Strong identity (indipendent UUID)
	QString m_title;	// Essential metadata
	std::uint8_t m_flags;	// Orthogonal states stuffed in a byte
};

#endif // ABSTRACTACTIVITY_H
