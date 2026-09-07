#include "views/utils/ActivityViewHelpers.h"

#include <QObject>

#include "views/utils/ActivityVisitors.h"

namespace app {
namespace ActivityViewHelpers {

QString typeLabel(const events::Activity& activity) {
  TypeLabelVisitor visitor;
  activity.accept(visitor);
  return visitor.label;
}

QString summaryLabel(const events::Activity& activity) {
  ActivitySummaryVisitor visitor;
  activity.accept(visitor);
  return visitor.summary;
}

QString recurrenceRuleLabel(const events::Activity& activity) {
  RecurrenceRuleVisitor visitor;
  activity.getGenerator().accept(visitor);
  return visitor.rule;
}

QString durationLabel(const events::Duration duration) {
  const qint64 minutes = duration.count() / 60;
  if (minutes < 60) {
    return QObject::tr("%1 min").arg(minutes);
  }
  if (minutes % 60 == 0) {
    return QObject::tr("%1 h").arg(minutes / 60);
  }
  return QObject::tr("%1 h %2 min").arg(minutes / 60).arg(minutes % 60);
}

} // namespace ActivityViewHelpers
} // namespace app
