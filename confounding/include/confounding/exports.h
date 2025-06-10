#pragma once

#ifdef CONFOUNDING_EXPORTS
#define CONFOUNDING_API __declspec(dllexport)
#else
#define CONFOUNDING_API __declspec(dllimport)
#endif
