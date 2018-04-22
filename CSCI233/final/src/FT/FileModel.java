package FT;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;

public class FileModel {
    public String fileName;
    public String fileType;
    public int fileSize;
    public String lastModified;
    public String address;
    public int port;

    public FileModel(String str) throws Exception {
        str = str.substring(1, str.length() - 1);
        String[] parsed = str.split(",");
        if (parsed.length != 6)
            throw new Exception("Illegal formatted string");
        this.fileName = parsed[0];
        this.fileType = parsed[1];
        this.fileSize = Integer.parseInt(parsed[2]);
        this.lastModified = parsed[3];
        this.address = parsed[4];
        this.port = Integer.parseInt(parsed[5]);
    }

    public FileModel(File file, String address, int port) {
        this.fileName = file.getName();
        int i = this.fileName.lastIndexOf('.');
        this.fileType = this.fileName.substring(i + 1);
        this.fileName = this.fileName.substring(0, i);
        this.fileSize = (int) file.length();
        Date last = new Date(file.lastModified());
        this.lastModified = new SimpleDateFormat("dd/MM/yy").format(last);
        this.address = address;
        this.port = port;
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
//                this.address.equals(that.address) &&
//                this.port == that.port;
    }

    @Override
    public String toString() {
        return "<" + fileName + "," + fileType + "," + fileSize + "," + lastModified + "," + address + "," + port + ">";
    }

}
