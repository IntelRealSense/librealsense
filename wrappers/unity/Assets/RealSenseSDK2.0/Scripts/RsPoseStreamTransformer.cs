using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Intel.RealSense;
using UnityEngine;

public class RsPoseStreamTransformer : MonoBehaviour
{
    [StructLayout(LayoutKind.Sequential)]
    public class RsPose
    {
        public Vector3 translation;
        public Vector3 velocity;
        public Vector3 acceleration;
        public Quaternion rotation;
        public Vector3 angular_velocity;
        public Vector3 angular_acceleration;
        public int tracker_confidence;
        public int mapper_confidence;
    }
    RsPose pose = new RsPose();


    public RsFrameProvider Source;

    FrameQueue q;

    void Start()
    {
        Source.OnStart += OnStartStreaming;
        Source.OnStop += OnStopStreaming;
    }

    private void OnStartStreaming(PipelineProfile profile)
    {
        q = new FrameQueue(1);
        Source.OnNewSample += OnNewSample;
    }


    private void OnStopStreaming()
    {
        Source.OnNewSample -= OnNewSample;

        if (q != null)
        {
            q.Dispose();
            q = null;
        }
    }


    private void OnNewSample(Frame f)
    {
        if (f.IsComposite)
        {
            using (var fs = f.As<FrameSet>())
            using (var poseFrame = fs.FirstOrDefault(Stream.Pose, Format.SixDOF))
                if (poseFrame != null)
                    q.Enqueue(poseFrame);
        }
        else
        {
            using (var p = f.Profile)
                if (p.Stream == Stream.Pose && p.Format == Format.SixDOF)
                    q.Enqueue(f);
        }
    }

    void Update()
    {
        if (q != null)
        {
            PoseFrame frame;
            if (q.PollForFrame<PoseFrame>(out frame))
                using (frame)
                {
                    frame.CopyTo(pose);

                    // Convert T265 coordinate system to Unity's
                    // see https://realsense.intel.com/how-to-getting-imu-data-from-d435i-and-t265/

                    var t = pose.translation;
                    t.Set(t.x, t.y, -t.z);

                    var e = pose.rotation.eulerAngles;
                    var r = Quaternion.Euler(-e.x, -e.y, e.z);

                    transform.localRotation = r;
                    transform.localPosition = t;
                }

        }
    }
}
