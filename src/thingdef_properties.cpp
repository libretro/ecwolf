#include "thingdef.h"

#define INT_PARAM(var, no) int64_t var = params[no].i
#define FLOAT_PARAM(var, no) double var = params[no].f;
#define STRING_PARAM(var, no) const char* var = params[no].s;

HANDLE_PROPERTY(health)
{
	INT_PARAM(health, 0);

	defaults->defaultHealth[0] = defaults->health = health;

	int currentSlope = 0;
	unsigned int lastValue = health;
	for(unsigned int i = 1;i < PARAM_COUNT;i++)
	{
		INT_PARAM(skillHealth, i);

		defaults->defaultHealth[i] = skillHealth;
		currentSlope = skillHealth - lastValue;
	}

	for(unsigned int i = PARAM_COUNT;i < 9;i++)
	{
		defaults->defaultHealth[i] = defaults->defaultHealth[i-1] + currentSlope;
	}
}

HANDLE_PROPERTY(speed)
{
	INT_PARAM(speed, 0);

	defaults->speed = speed;
}

#define DEFINE_PROP(name, class, params) { class::__StaticClass, #name, #params, __Handler_##name }
extern const PropDef properties[NUM_PROPERTIES] =
{
	DEFINE_PROP(health, AActor, I_IIIIIIII),
	DEFINE_PROP(speed, AActor, I)
};
