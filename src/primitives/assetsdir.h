
#ifndef RAIN_ASSETSDIR_H
#define RAIN_ASSETSDIR_H

#include <fs.h>
#include <primitives/asset.h>
#include <script/standard.h>
#include <map>

class CBlock;
class CDataStream;
class CClientUIInterface;

extern std::vector<CAsset> _initAssets(std::string& signingkey);
CAmountMap populateMap(CAmount amount);
extern CAsset getAsset(std::string asset);

#endif // RAIN_ASSETSDIR_H
