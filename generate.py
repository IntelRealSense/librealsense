import sys
reload(sys)
import json;
import os;
from pprint import pprint;
sys.setdefaultencoding('utf-8')

if (len(sys.argv) == 3):
    template = sys.argv[1];
    data = sys.argv[2];

    with open(template, 'r') as myfile:
        content = myfile.read();

    with open(data) as f:
        data_content = json.load(f)

    body = "";
    index = 0;
    total = len(data_content["items"]);
    for obj in data_content["items"]:
        if (index % 3 == 0):
            if (index > 0):
                body = body + "\t\t</div>\n";
            if (index < total - 1):
                body = body + "\t\t<div class=\"row\">\n";
        else:
            if (index == total - 1):
                body = body + "\t\t</div>\n";
                
        body = body + "\t\t\t<div class=\"col-md-4\">\n";
        body = body + "\t\t\t\t<table class='t-table table borderless' border=1 bordercolor=#3E4D59>\n";
        body = body + "\t\t\t\t\t<tr>\n";
        body = body + "\t\t\t\t\t\t<td style='height: 40px; background-color: #1b2125'>\n";
        body = body + "\t\t\t\t\t\t\t" + obj["title"] + "\n";
        body = body + "\t\t\t\t\t\t</td>\n";
        body = body + "\t\t\t\t\t</tr>\n";
        body = body + "\t\t\t\t\t<tr style='height: 180px'>\n";
        body = body + "\t\t\t\t\t\t<td class=\"nopadding\">\n";
        body = body + "\t\t\t\t\t\t<a href=\"" + obj["link"] + "\" title=\"" + obj["link_title"] + "\">\n";
        body = body + "\t\t\t\t\t\t<img src=\"" + obj["static_thumbnail"] + "\" onmouseenter=\"this.src='" + obj["active_thumbnail"] + "';\" onmouseout=\"this.src='" + obj["static_thumbnail"] + "';\" width=100% />\n";
        body = body + "\t\t\t\t\t\t</a>\n";
        body = body + "\t\t\t\t\t\t</td>\n";
        body = body + "\t\t\t\t\t</tr>\n";
        body = body + "\t\t\t\t\t<tr>\n";
        body = body + "\t\t\t\t\t\t<td>\n";
        body = body + "\t\t\t\t\t\t<div class='scroll-container' style='padding: 5px;'>\n";
        body = body + obj["body"];
        body = body + "</div>\n";
        body = body + "\t\t\t\t\t\t</td>\n";
        body = body + "\t\t\t\t\t</tr>\n";
        body = body + "\t\t\t\t</table>\n";
        body = body + "\t\t\t</div>\n";
        
        index = index+1;

    content = content.replace("<!-- Content -->", body);

    output_filename = os.path.basename(template);
    with open(output_filename, "w") as text_file:
        text_file.write(content);

    print("Done!");

else:
    print("Usage: python generate.py [template.html] [data.json]");
