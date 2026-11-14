import os

os.makedirs('generated', exist_ok=True)

with open('generated/webview.hpp', 'w') as f:
    f.write('#pragma once\n')
    f.write('#include <string>\n')
    f.write('std::string WEBVIEW_HTML = "<h1>Hello from Webview</h1>";\n')