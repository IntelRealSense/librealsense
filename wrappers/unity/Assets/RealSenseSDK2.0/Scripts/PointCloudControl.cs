using UnityEngine;

public class PointCloudControl : MonoBehaviour {
    public float _zoomSpeedFactor = 100;
    public float _rotateSpeedFactor = 0.1f;
    public float _moveSpeedFactor = 10;
    public Vector3 _rotateAround = new Vector3(0, 0, 1000);
    private Vector3 prevMousePosition;

    // Use this for initialization
    void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {

        if(prevMousePosition.magnitude == 0)
            prevMousePosition = Input.mousePosition;

        if (Input.anyKey == false)
        {
            float scroll = Input.GetAxis("Mouse ScrollWheel") * _zoomSpeedFactor;
            transform.Translate(Vector3.forward * scroll);
        }

        if (Input.GetMouseButton(0))
        {
            var currMousePosition = Input.mousePosition;
            var diff = currMousePosition - prevMousePosition;
            prevMousePosition = currMousePosition;
            diff *= _rotateSpeedFactor;
            if (diff.magnitude > 10)
                return;
            transform.RotateAround(_rotateAround, new Vector3(0, 1, 0), diff.x);
            transform.RotateAround(_rotateAround, new Vector3(1, 0, 0), -diff.y);

        }

        if (Input.GetMouseButton(1))
        {
            var currMousePosition = Input.mousePosition;
            var diff = currMousePosition - prevMousePosition;
            prevMousePosition = currMousePosition;
            diff *= _rotateSpeedFactor;
            transform.RotateAround(_rotateAround, new Vector3(1, 0, 0), diff.y);
        }

        if (Input.GetKey(KeyCode.UpArrow))
        {
            transform.Translate(Vector3.up * _moveSpeedFactor);
        }

        if (Input.GetKey(KeyCode.DownArrow))
        {
            transform.Translate(Vector3.down * _moveSpeedFactor);
        }

        if (Input.GetKey(KeyCode.LeftArrow))
        {
            transform.Translate(Vector3.left * _moveSpeedFactor);
        }

        if (Input.GetKey(KeyCode.RightArrow))
        {
            transform.Translate(Vector3.right * _moveSpeedFactor);
        }
    }
}
