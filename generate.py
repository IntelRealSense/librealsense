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

    pages = [];
    body = "";
    index = 0;
    total = len(data_content["items"]);

    items_per_page = 9;

    max_this_page = min([ total, items_per_page]);
    
    for obj in data_content["items"]:
        if (index % 3 == 0):
            if (index > 0):
                body = body + "\t\t</div>\n";

        if (index == max_this_page):
            index = 0;
            pages.append(body);
            body = "";
            total = total - items_per_page;
            max_this_page = min([ total, items_per_page]);

        if (index % 3 == 0):
            if (index < max_this_page):
                body = body + "\t\t<div class=\"row\">\n";
                
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

    pages.append(body);
    output_filename = os.path.basename(template);

    page_id = 1;
    
    for b in pages:

        pagination = "";
        if (len(data_content["items"]) > items_per_page):
            page_count = len(pages);
            pagination = "<ul class=\"pagination\">\n";
            if (page_id > 1):
                
                pagination = pagination + "<li class=\"page-item\">\n";
                pagination = pagination + "<a class=\"page-link\" href=\"" + last_file + "\" aria-label=\"Previous\">\n";
                pagination = pagination + "<span aria-hidden=\"true\">&laquo;</span>\n";
                pagination = pagination + "<span class=\"sr-only\">Previous</span>\n";
                pagination = pagination + "</a></li>\n";
            else:
                pagination = pagination + "<li class=\"page-item disabled\">\n";
                pagination = pagination + "<a class=\"page-link\" href=\"#\" aria-label=\"Previous\">\n";
                pagination = pagination + "<span aria-hidden=\"true\">&laquo;</span>\n";
                pagination = pagination + "<span class=\"sr-only\">Previous</span>\n";
                pagination = pagination + "</a></li>\n";

            for idx in range(1, page_count + 1):
                if (idx == page_id):
                    pagination = pagination + "<li class=\"page-item active\">\n";
                else:
                    pagination = pagination + "<li class=\"page-item\">\n"
                pagination = pagination + "<a class=\"page-link\" href=\"";
                if (idx == 1):
                    pagination = pagination + os.path.basename(template);
                else:
                    pagination = pagination + os.path.basename(template).replace(".html", "-" + str(idx) + ".html");
                pagination = pagination + "\">" + str(idx) + "</a>\n";
                pagination = pagination + "</li>"

            if (page_id < page_count):
                
                pagination = pagination + "<li class=\"page-item\">\n";
                pagination = pagination + "<a class=\"page-link\" href=\"" + os.path.basename(template).replace(".html", "-" + str(page_id + 1) + ".html") + "\" aria-label=\"Previous\">\n";
                pagination = pagination + "<span aria-hidden=\"true\">&raquo;</span>\n";
                pagination = pagination + "<span class=\"sr-only\">Next</span>\n";
                pagination = pagination + "</a></li>\n";
            else:
                pagination = pagination + "<li class=\"page-item disabled\">\n";
                pagination = pagination + "<a class=\"page-link\" href=\"#\" aria-label=\"Previous\">\n";
                pagination = pagination + "<span aria-hidden=\"true\">&raquo;</span>\n";
                pagination = pagination + "<span class=\"sr-only\">Next</span>\n";
                pagination = pagination + "</a></li>\n";
        
        new_c = content.replace("<!-- Content -->", b);
        new_c = new_c.replace("<!-- Pagination -->", pagination);

        with open(output_filename, "w") as text_file:
            text_file.write(new_c);

        page_id = page_id + 1;
        last_file = output_filename;
        output_filename = os.path.basename(template).replace(".html", "-" + str(page_id) + ".html");
        

    print("Done!");

else:
    print("Usage: python generate.py [template.html] [data.json]");
