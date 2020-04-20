using UnityEngine;
using UnityEngine.EventSystems;

public class OrbitCameraControl : MonoBehaviour
{
    public float _zoomSpeedFactor = 2;
    public float _rotateSpeedFactor = 0.1f;
    public float _moveSpeedFactor = 0.1f;
    public Vector3 _rotateAround = new Vector3(0, 0, 1);
    private Vector3 prevMousePosition;

    Camera cam;

    [HideInInspector]
    public Texture2D OrbitCursor;
    [HideInInspector]
    public Texture2D PanCursor;
    [HideInInspector]
    public Texture2D ZoomCursor;
    readonly Vector2 cursorOffset = new Vector2(32, 32);

    void Start()
    {
        cam = GetComponent<Camera>();

        // AssetBundle workaround
#if UNITY_EDITOR
        OrbitCursor.alphaIsTransparency = true;
        PanCursor.alphaIsTransparency = true;
        ZoomCursor.alphaIsTransparency = true;
#endif
    }

    void OnDisable()
    {
        Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
    }

    void Update()
    {
        if (!isActiveAndEnabled)
            return;

        if (!Application.isFocused)
            return;

        Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);

        if (EventSystem.current && EventSystem.current.IsPointerOverGameObject())
            return;

        var currMousePosition = Input.mousePosition;
        var diff = currMousePosition - prevMousePosition;
        prevMousePosition = currMousePosition;

        // Zoom / FOV
        if (Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift))
        {
            float scroll = Input.GetAxis("Mouse ScrollWheel");
            cam.fieldOfView = Mathf.Clamp(cam.fieldOfView + scroll * 20f, 1f, 179f);
        }
        else
        {
            float scroll = Input.GetAxis("Mouse ScrollWheel") * _zoomSpeedFactor;
            var z = Vector3.forward * scroll;
            transform.Translate(z);
            _rotateAround -= z;
            Cursor.SetCursor(scroll == 0 ? null : ZoomCursor, Vector2.zero, CursorMode.Auto);
        }

        bool ctrlAlt = Input.GetKey(KeyCode.LeftAlt) && Input.GetKey(KeyCode.LeftControl);

        // Orbit
        if (!ctrlAlt)
        {
            if (Input.GetMouseButtonDown(0))
            {
                prevMousePosition = Input.mousePosition;
            }
            else
            if (Input.GetMouseButton(0))
            {
                diff *= _rotateSpeedFactor;
                transform.Translate(_rotateAround);
                transform.Rotate(Vector3.right, -diff.y);
                transform.Rotate(Vector3.up, diff.x, Space.World);
                transform.Translate(-_rotateAround);

                Cursor.SetCursor(OrbitCursor, cursorOffset, CursorMode.Auto);
            }
        }


        // Look / Zoom
        if (Input.GetMouseButtonDown(1))
        {
            prevMousePosition = Input.mousePosition;
        }
        else
        if (Input.GetMouseButton(1))
        {
            if (Input.GetKey(KeyCode.LeftAlt) || Input.GetKey(KeyCode.LeftAlt))
            {
                var s = Mathf.Sign(Vector3.Dot(diff, Vector3.right));

                // var z = Vector3.zero;
                var z = Vector3.forward * diff.magnitude * 0.1f * s * Time.deltaTime;
                transform.Translate(z);
                _rotateAround -= z;

                Cursor.SetCursor(ZoomCursor, cursorOffset, CursorMode.Auto);
            }
            else
            {
                diff *= _rotateSpeedFactor;
                transform.Rotate(Vector3.right, -diff.y);
                transform.Rotate(Vector3.up, diff.x, Space.World);

                Cursor.SetCursor(OrbitCursor, cursorOffset, CursorMode.Auto);
            }
        }



        // Pan
        if (Input.GetMouseButtonDown(2) || (ctrlAlt && Input.GetMouseButtonDown(0)))
        {
            prevMousePosition = Input.mousePosition;
        }
        else
        if (Input.GetMouseButton(2) || (ctrlAlt && Input.GetMouseButton(0)))
        {
            diff *= Time.deltaTime * _moveSpeedFactor;
            transform.Translate(-diff.x, -diff.y, 0);

            Cursor.SetCursor(PanCursor, cursorOffset, CursorMode.Auto);
        }

        // Move
        var m = new Vector3(Input.GetAxis("Horizontal"), 0, Input.GetAxis("Vertical"));
        transform.Translate(m * _moveSpeedFactor);
    }


    public void Reset()
    {
        transform.SetPositionAndRotation(Vector3.zero, Quaternion.identity);
        _rotateAround = Vector3.forward;

    }
}
