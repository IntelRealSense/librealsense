package com.intel.realsense.librealsense;

public class RegionOfInterest {
    public int minX;
    public int minY;
    public int maxX;
    public int maxY;

    public RegionOfInterest(){}
    public RegionOfInterest(int minX,int minY, int maxX, int maxY){
        this.minX = minX;
        this.minY = minY;
        this.maxX = maxX;
        this.maxY = maxY;
    }
}
