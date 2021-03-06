package iota.common.definitions;

import iota.common.functions.IFunction;

import java.io.BufferedReader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Created by alist on 9/07/2017.
 */
class TxtParser implements IFuncDefFileParser {
    private Path file;

    public TxtParser(Path fileIn) {
        this.file = fileIn;
    }

    @Override
    public IFunction parseFile() {
        return createDef(createLineMap());
    }

    private HashMap<String, List<String>> createLineMap() {
        HashMap<String, List<String>> lmap = new HashMap<>();
        try (BufferedReader br = Files.newBufferedReader(file)) {
            String line = null;
            while ((line = br.readLine()) != null) {
                if (line.startsWith("#")) {
                    String key = line.substring(0, line.indexOf(" "));
                    String val = line.substring(line.indexOf(" "));
                    List<String> vals = lmap.get(key);
                    if (vals == null) {
                        vals = new ArrayList<String>();
                    }
                    vals.add(val.trim());
                    lmap.put(key, vals);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return lmap;
    }


    private IFunction createDef(HashMap<String, List<String>> lineMap) {
        IFunction def;
        def = new FileBasedFuncDef(lineMap);
        return def;
    }
}
