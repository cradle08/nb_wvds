package com.cadre.wvds.sniffer;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.commons.io.FileUtils;

import com.alibaba.fastjson.JSON;

public class TraceFileManager {

    private String fname;

    private List<TraceFile> files = new ArrayList<TraceFile>();
    private List<MVCView> views = new ArrayList<MVCView>();

    public TraceFileManager(String fname) {
        this.fname = fname;
        load(fname);
    }

    public void load(String fname) {
        File file = new File(fname);
        try {
            String text = FileUtils.readFileToString(file);
            files = JSON.parseArray(text, TraceFile.class);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    public void save() {
        save(this.fname);
    }

    public void save(String fname) {
        File file = new File(fname);
        try {
            String text = JSON.toJSONString(files);
            FileUtils.write(file, text);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    public List<TraceFile> getFiles() {
        return files;
    }

    public void addFile(TraceFile f) {
        files.add(f);
        for (MVCView v : views)
            v.dataAdded(f);
    }

    public void removeFile(String fname) {
        for (Iterator<TraceFile> it = files.iterator(); it.hasNext(); ) {
            TraceFile f = it.next();
            if (f.getPath().equals(fname)) {
                it.remove();
                for (MVCView v : views)
                    v.dataRemoved(f);
            }
        }
    }

    public void addView(MVCView v) {
        views.add(v);
    }

}
