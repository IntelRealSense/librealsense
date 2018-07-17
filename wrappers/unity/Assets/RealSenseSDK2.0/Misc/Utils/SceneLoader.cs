using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneLoader : MonoBehaviour
{
    private static bool created = false;

    void Awake()
    {
        if (!created)
        {
            AssetBundle.LoadFromFile(Path.Combine(Application.streamingAssetsPath, "sample_assets"));
            AssetBundle.LoadFromFile(Path.Combine(Application.streamingAssetsPath, "sample_scenes"));
            created = true;
        }
    }

    public void MainScene()
    {
        SceneManager.LoadScene("StartHere");
    }

    public void LoadScene(string name)
    {
        SceneManager.LoadScene("SampleSceneUI");
        LoadSceneAdditive(name);
    }

    public void LoadSceneAdditive(string name)
    {
        SceneManager.LoadScene(name, LoadSceneMode.Additive);
    }

    public void Quit()
    {
        Application.Quit();
    }

    public void OpenURL(string url)
    {
        Application.OpenURL(url);
    }
}
