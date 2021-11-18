char INDEX[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Bachelorarbeit</title>
        <style> 
            iframe {
                height: 500px;
                width: 100%;
            }
            .btn { 
                text-align: center;
                color: white;
                background-color: #AC3C3C;
                height: 50px;
                min-width: 120px;
                max-width: 300px;
                width: 100%;
                display: block;
                margin: 0 auto;
            }
            #container {
                display: flex;
                flex-flow: row wrap;
                justify-content: space-around;
                width: 100%;
            }
            </style>
    </head>
    <body>
        <button onclick="location.href='/add/'" class="btn" style="background-color: #313531; border: BORDER_COLOR 3px solid;">Add cam</button>
        <div id="container">
        </div>
        <script>
            var container = document.getElementById("container");
            var listCamIP = CAM_IP_LIST;

            for(var i = 0; i < listCamIP.length; i++){
                var remLocation = "/remove/" + i + "/";
                //create iframe with cam ip inside
                var elemDiv = document.createElement("div");
                var elemIframe = document.createElement("iframe");
                elemDiv.appendChild(elemIframe);
                var attrSrc = document.createAttribute("src");
                attrSrc.value = "http://" + listCamIP[i] + "/";
                elemIframe.setAttributeNode(attrSrc);
                //create remove button with index in path
                var elemButton = document.createElement("button");
                var elemClass = document.createAttribute("class");
                var elemOnclick = document.createAttribute("onclick");
                elemOnclick.value = "location.href=" + "'" + remLocation + "'";
                elemButton.setAttributeNode(elemOnclick);
                elemClass.value = "btn";
                elemButton.setAttributeNode(elemClass);
                elemButton.innerHTML = "Remove cam";
                container.appendChild(elemDiv);
                elemDiv.appendChild(elemButton);              
            }
        </script>
    </body>
</html>
)=====";
