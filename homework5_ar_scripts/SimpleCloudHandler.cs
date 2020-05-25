using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Windows.Forms;
using System;
using System.IO;
using Pathfinding.Serialization.JsonFx;
using System.Net;
using System.Security.Cryptography;

using Vuforia;
using System.Text;

public class PostNewTrackableRequest
{
    public string name;
    public float width;
    public string image;
    public string application_metadata;
}


public class SimpleCloudHandler : MonoBehaviour, IObjectRecoEventHandler
{
    public ImageTargetBehaviour ImageTargetTemplate;
    public bool showDialog = false;

    private string access_key = "YOUR SERVER ACCESS KEY";
    private string secret_key = "YOUR SERVER SECRET KEY";
    private string url = @"https://vws.vuforia.com";

    private CloudRecoBehaviour mCloudRecoBehaviour;
    private bool mIsScanning = false;
    private string mTargetMetadata = "";
    // Use this for initialization 
    void Start()
    {
        // register this event handler at the cloud reco behaviour 
        mCloudRecoBehaviour = GetComponent<CloudRecoBehaviour>();

        if (mCloudRecoBehaviour)
        {
            mCloudRecoBehaviour.RegisterEventHandler(this);
        }
    }

    public void OnInitialized(TargetFinder targetFinder)
    {
        Debug.Log("Cloud Reco initialized");
    }
    public void OnInitError(TargetFinder.InitState initError)
    {
        Debug.Log("Cloud Reco init error " + initError.ToString());
    }
    public void OnUpdateError(TargetFinder.UpdateState updateError)
    {
        Debug.Log("Cloud Reco update error " + updateError.ToString());
    }

    public void OnStateChanged(bool scanning)
    {
        mIsScanning = scanning;
        if (scanning)
        {
            // clear all known trackables
            var tracker = TrackerManager.Instance.GetTracker<ObjectTracker>();
            tracker.GetTargetFinder<ImageTargetFinder>().ClearTrackables(false);
        }
    }

    // Here we handle a cloud target recognition event
    public void OnNewSearchResult(TargetFinder.TargetSearchResult targetSearchResult)
    {
        Debug.Log("New search result");
        TargetFinder.CloudRecoSearchResult cloudRecoSearchResult =
            (TargetFinder.CloudRecoSearchResult)targetSearchResult;
        // do something with the target metadata
        string tempVideoPath = "video.mp4";

        string stringVideo = cloudRecoSearchResult.MetaData;
        Debug.Log("meta: " + stringVideo);
        // var bytesVideo = Encoding.ASCII.GetBytes(stringVideo);
        // File.WriteAllBytes(tempVideoPath, bytesVideo);
        Debug.Log("Downloaded string size = " + stringVideo.Length.ToString());
        // Debug.Log("Downloaded video size = " + bytesVideo.Length.ToString() + " bytes");

        GameObject imageTargetPlane = GameObject.Find("Plane");
        var videoPlayer = imageTargetPlane.GetComponent<UnityEngine.Video.VideoPlayer>();
        videoPlayer.url = stringVideo; // tempVideoPath;

        // stop the target finder (i.e. stop scanning the cloud)
        mCloudRecoBehaviour.CloudRecoEnabled = false;

        if (ImageTargetTemplate)
        {
            // enable the new result with the same ImageTargetBehaviour: 
            ObjectTracker tracker = TrackerManager.Instance.GetTracker<ObjectTracker>();
            tracker.GetTargetFinder<ImageTargetFinder>().EnableTracking(targetSearchResult, ImageTargetTemplate.gameObject);
        }
    }

    public void CreateCloudVideoImageTarget(string picPath, string videoPath)
    {
        StartCoroutine(PostNewTarget(picPath, videoPath));
    }

    IEnumerator PostNewTarget(string picPath, string videoPath) {
        string requestPath = "/targets";
        string serviceURI = url + requestPath;
        string httpAction = "POST";
        string contentType = "application/json";
        string date = string.Format("{0:r}", DateTime.Now.ToUniversalTime());

        byte[] imageTarget = File.ReadAllBytes(picPath);
        byte[] video = File.ReadAllBytes(videoPath);
        Debug.Log("Uploaded video size = " + video.Length.ToString() + " bytes");

        PostNewTrackableRequest model = new PostNewTrackableRequest();
        model.name = Path.GetFileName(picPath);
        model.width = 3.0f;
        model.image = Convert.ToBase64String(imageTarget);
        model.application_metadata = Convert.ToBase64String(video);
        string requestBody = JsonWriter.Serialize(model);

        var bytesVideo = Convert.FromBase64String(model.application_metadata);
        File.WriteAllBytes("translated_video.mp4", bytesVideo);

        WWWForm form = new WWWForm();

        var headers = form.headers;
        byte[] rawData = form.data;
        headers["host"] = url;
        headers["date"] = date;
        headers["content-type"] = contentType;

        HttpWebRequest httpWReq = (HttpWebRequest)HttpWebRequest.Create(serviceURI);

        MD5 md5 = MD5.Create();
        var contentMD5bytes = md5.ComputeHash(System.Text.Encoding.ASCII.GetBytes(requestBody));
        System.Text.StringBuilder sb = new System.Text.StringBuilder();
        for (int i = 0; i < contentMD5bytes.Length; i++)
        {
            sb.Append(contentMD5bytes[i].ToString("x2"));
        }

        string contentMD5 = sb.ToString();

        string stringToSign = string.Format("{0}\n{1}\n{2}\n{3}\n{4}", httpAction, contentMD5, contentType, date, requestPath);

        HMACSHA1 sha1 = new HMACSHA1(System.Text.Encoding.ASCII.GetBytes(secret_key));
        byte[] sha1Bytes = System.Text.Encoding.ASCII.GetBytes(stringToSign);
        MemoryStream stream = new MemoryStream(sha1Bytes);
        byte[] sha1Hash = sha1.ComputeHash(stream);
        string signature = System.Convert.ToBase64String(sha1Hash);

        headers["authorization"] = string.Format("VWS {0}:{1}", access_key, signature);

        Debug.Log("<color=green>Signature: " + signature + "</color>");

        WWW request = new WWW(serviceURI, System.Text.Encoding.UTF8.GetBytes(JsonWriter.Serialize(model)), headers);
        yield return request;

        if (request.error != null)
        {
            Debug.Log("request error: " + request.error);
        }
        else
        {
            Debug.Log("request success");
            Debug.Log("returned data" + request.text);
        }
    }

    void Update()
    {
        if (showDialog == true)
        {
            OpenFileDialog picDialog = new OpenFileDialog
            {
                Multiselect = true,
                Filter = "All files (*.*)|*.*"
            };


            if (picDialog.ShowDialog() == DialogResult.OK)
            {
                string picPath = picDialog.FileName;
                Debug.Log("Dialog was confirmed: " + picPath);

                OpenFileDialog videoDialog = new OpenFileDialog
                {
                    Multiselect = true,
                    Filter = "All files (*.*)|*.*"
                };

                if (videoDialog.ShowDialog() == DialogResult.OK)
                {
                    string videoPath = videoDialog.FileName;
                    Debug.Log("Dialog was confirmed: " + videoPath);

                    CreateCloudVideoImageTarget(picPath, videoPath);
                }
            }
        }
        showDialog = false;
    }

    void OnGUI()
    {
        // Display current 'scanning' status
        GUI.Box(new Rect(100, 50, 200, 50), mIsScanning ? "Scanning" : "Not scanning");
        // Display metadata of latest detected cloud-target
        // GUI.Box(new Rect(100, 150, 200, 50), "Metadata: " + mTargetMetadata);
        // If not scanning, show button
        // so that user can restart cloud scanning
        if (!mIsScanning)
        {
            if (GUI.Button(new Rect(100, 250, 200, 50), "Restart Scanning"))
            {
                // Restart TargetFinder
                mCloudRecoBehaviour.CloudRecoEnabled = true;
            }
        }

        if (GUI.Button(new Rect(100, 350, 200, 50), "Upload pic and video"))
        {
            Debug.Log("Add pics and video button push");
            showDialog = true;
        }
    }
}
 