package iota.common.functions;

import iota.client.model.EspDevice;
import iota.common.Constants;
import iota.common.definitions.IStateItem;

import java.util.List;

public class HubInternal implements IFunction {

    List<IStateItem> attachedFunctions;
    List<IStateItem> debugParameters;


    public HubInternal(EspDevice device) {

    }


    @Override
    public String getDisplayName() {
        return null;
    }

    @Override
    public short getFuncId() {
        return Constants.FID_HUB;
    }

    @Override
    public int handleReceivedData(List<Byte> recievedData) {
        return 0;
    }

    @Override
    public List<IStateItem> getStateItems() {
        return null;
    }


}
