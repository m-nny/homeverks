package FT;

public class FileModel {
    String fileName, fileType;
    int fileSize;
    String lastModified;
    String ipAddress;
    int port;

    public FileModel(String str) throws Exception {
        str = str.substring(1, str.length() - 1);
        String[] parsed = str.split(",");
        if (parsed.length != 6)
            throw new Exception("Illegal formatted string");
        fileName = parsed[0];
        fileType = parsed[1];
        fileSize = Integer.parseInt(parsed[2]);
        lastModified = parsed[3];
        ipAddress = parsed[4];
        port = Integer.parseInt(parsed[5]);
    }

    @Override
    public boolean equals(Object aThat) {
        if (this == aThat)
            return true;
        if (!(aThat instanceof FileModel))
            return false;
        FileModel that = (FileModel) aThat;
        return this.toString().equals(that.toString());
//        return this.fileName.equals(that.fileName) &&
//                this.fileType.equals(that.fileType) &&
//                this.fileSize == that.fileSize &&
//                this.lastModified.equals(that.lastModified) &&
//                this.ipAddress.equals(that.ipAddress) &&
//                this.port == that.port;
    }

    @Override
    public String toString() {
        return "<" + fileName + ", " + fileType + ", " + fileSize + "," + lastModified + "," + ipAddress + "," + port + ">";
    }

}
