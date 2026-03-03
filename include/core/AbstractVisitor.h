#ifndef ACTIVITYVISITOR_H
#define ACTIVITYVISITOR_H

// --- FORWARD DECL OF THE CONCRETE CLASSES ---
// Note: only references are used here
class ReadingSession;

class ActivityVisitor {
public:
	virtual ~ActivityVisitor() = default;

	// --- OVERLOADING WITH DOUBLE DISPATCH ---
	// The compiler will basically choose the correct function based on the type
	// statically! This is passed via *this in the method
	virtual void visit(ReadingSession& activity) = 0;
};

#endif // ACTIVITYVISITOR_H
