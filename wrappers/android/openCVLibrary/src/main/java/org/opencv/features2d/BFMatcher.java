
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;



// C++: class BFMatcher
//javadoc: BFMatcher
public class BFMatcher extends DescriptorMatcher {

    protected BFMatcher(long addr) { super(addr); }


    //
    // C++:   BFMatcher(int normType = NORM_L2, bool crossCheck = false)
    //

    //javadoc: BFMatcher::BFMatcher(normType, crossCheck)
    public   BFMatcher(int normType, boolean crossCheck)
    {
        
        super( BFMatcher_0(normType, crossCheck) );
        
        return;
    }

    //javadoc: BFMatcher::BFMatcher()
    public   BFMatcher()
    {
        
        super( BFMatcher_1() );
        
        return;
    }


    //
    // C++: static Ptr_BFMatcher create(int normType = NORM_L2, bool crossCheck = false)
    //

    //javadoc: BFMatcher::create(normType, crossCheck)
    public static BFMatcher create(int normType, boolean crossCheck)
    {
        
        BFMatcher retVal = new BFMatcher(create_0(normType, crossCheck));
        
        return retVal;
    }

    //javadoc: BFMatcher::create()
    public static BFMatcher create()
    {
        
        BFMatcher retVal = new BFMatcher(create_1());
        
        return retVal;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++:   BFMatcher(int normType = NORM_L2, bool crossCheck = false)
    private static native long BFMatcher_0(int normType, boolean crossCheck);
    private static native long BFMatcher_1();

    // C++: static Ptr_BFMatcher create(int normType = NORM_L2, bool crossCheck = false)
    private static native long create_0(int normType, boolean crossCheck);
    private static native long create_1();

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
