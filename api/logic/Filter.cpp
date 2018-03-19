#include "Filter.h"

Filter::~Filter(){}

ContainsFilter::ContainsFilter(const QString& pattern) : pattern(pattern){}
ContainsFilter::~ContainsFilter(){}
bool ContainsFilter::accepts(const QString& value)
{
	return value.contains(pattern);
}

ExactFilter::ExactFilter(const QString& pattern) : pattern(pattern){}
ExactFilter::~ExactFilter(){}
bool ExactFilter::accepts(const QString& value)
{
	return value.contains(pattern);
}

RegexpFilter::RegexpFilter(const QString& regexp, bool invert)
	:invert(invert)
{
	pattern.setPattern(regexp);
	pattern.optimize();
}
RegexpFilter::~RegexpFilter(){}
bool RegexpFilter::accepts(const QString& value)
{
	auto match = pattern.match(value);
	bool matched = match.hasMatch();
	return invert ? (!matched) : (matched);
}
