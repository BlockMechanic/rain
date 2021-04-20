
#include <primitives/confidential.h>

#include <tinyformat.h>

void CConfidentialAsset::SetToAsset(const CAsset& asset)
{
    vchCommitment.clear();
    vchCommitment.reserve(nExplicitSize);
    vchCommitment.push_back(1);
    std::vector<unsigned char> buffer(sizeof(asset));
    memcpy(&buffer[0], &asset, sizeof(asset));
    vchCommitment.insert(vchCommitment.end(), buffer.begin(), buffer.end());
}

void CConfidentialValue::SetToAmount(const CAmount amount)
{
    vchCommitment.resize(nExplicitSize);
    vchCommitment[0] = 1;
    WriteBE64(&vchCommitment[1], amount);
}

std::string CAssetIssuance::ToString() const
{
    std::string str;
    str += "CAssetIssuance(";
    str += assetBlindingNonce.ToString();
    str += ", ";
    str += assetEntropy.ToString();
    if (!nAmount.IsNull())
        str += strprintf(", amount=%s", (nAmount.IsExplicit() ? strprintf("%d.%08d", nAmount.GetAmount() / COIN, nAmount.GetAmount() % COIN) : std::string("CONFIDENTIAL")));
    if (!nInflationKeys.IsNull())
        str += strprintf(", inflationkeys=%s", (nInflationKeys.IsExplicit() ? strprintf("%d.%08d", nInflationKeys.GetAmount() / COIN, nInflationKeys.GetAmount() % COIN) : std::string("CONFIDENTIAL")));
    str += ")\n";
    return str;
}