#ifndef __TOOL_KIT_HPP__
#define __TOOL_KIT_HPP__

#include <string>
#include <unordered_map>
#include <vector>

typedef int sceneId_t;

/**
 *TODO these should be ints or something, make a system to get unique ints based on type or something
 * otherwise application defined values can overlap with library defined values
 */
typedef std::string sceneType_t;
typedef std::string actionType_t;
typedef std::string itemType_t;

#endif