import QtQuick 2.12
import QtQuick.Controls 2.12

SwipeView {
    id: chartViews
    
    StakeChart {
        id:stakeChart    
    }
    
    SupplyChart {
        id:supplyChart
    }
    
    DistributionChart{
        id:distributionChart
    }

}
