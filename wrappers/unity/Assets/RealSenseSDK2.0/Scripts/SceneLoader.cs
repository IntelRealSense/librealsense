using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneLoader : MonoBehaviour
{


    public void LoadScene(Scene s)
    {
        SceneManager.LoadScene(s.name, LoadSceneMode.Additive);
    }

    public void LoadScene(string s)
    {
        SceneManager.LoadScene(s, LoadSceneMode.Additive);
    }
}
