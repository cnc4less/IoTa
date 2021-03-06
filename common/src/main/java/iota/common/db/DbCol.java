package iota.common.db;

/**
 * Created by alist on 10/07/2017.
 */
public class DbCol {
    private String colName;
    private String colType;
    private boolean isPrimaryKey = false;
    private boolean isAutoIncrement = false;
    private boolean isNotNull = false;

    public DbCol(String colName, String colType, String[] modifiers) {
        this.colType = colType;
        this.colName = colName;
        for (String s : modifiers) {
            if (s.equalsIgnoreCase("ai")) {
                this.isAutoIncrement = true;
            } else if (s.equalsIgnoreCase("pk")) {
                this.isPrimaryKey = true;
                this.isNotNull = true;
            } else if (s.equalsIgnoreCase("nn")) {
                this.isNotNull = true;
            }
        }
    }

    public String getColName() {
        return colName;
    }

    public String getColType() {
        return colType;
    }

    public boolean getIsPrimaryKey() {
        return isPrimaryKey;
    }

    public boolean getIsAutoInc() {
        return isAutoIncrement;
    }

    public boolean getIsNotNull() {
        return isNotNull;
    }
}
