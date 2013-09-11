#include "OneSixRule.h"

RuleAction RuleAction_fromString(QString name)
{
	if(name == "allow")
		return Allow;
	if(name == "disallow")
		return Disallow;
	return Defer;
}