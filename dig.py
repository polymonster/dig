import cgu
import sys
import datetime
import juno
import redeye
import json

from google.oauth2 import service_account
from google.auth.transport.requests import AuthorizedSession

# member wise merge 2 dicts, second will overwrite dest
def merge_dicts(dest, second):
    for k, v in second.items():
        if type(v) == dict:
            if k not in dest or type(dest[k]) != dict:
                dest[k] = dict()
            merge_dicts(dest[k], v)
        else:
            dest[k] = v


# find and parse a named html element
def find_parse_elem(text: str, pos: int, ident: str, end_str: str, reverse=False):
    if reverse:
        pos = text[0:pos].rfind(ident)
    else:
        pos = pos + text[pos:].find(ident)
        assert(pos != -1)

    # parse the start element
    end = cgu.enclose("<", ">", text, pos)

    # parse the rest of the body
    last = end-1
    if text[last] != end_str:
        end = last + text[last:].find(end_str)

    # return the result and the iter pos
    elem =  text[pos:end]
    return (end, elem)


# get the value of a html element, ie <a href="value"
def get_value(elem: str, ident: str):
    pos = elem.find(ident + "=")
    if pos != -1:
        val_start, encloser = cgu.find_first(elem, ["\"", "'"], pos)
        val_start = val_start+1
        val_end = val_start + elem[val_start:].find(encloser)
        return elem[val_start:val_end]
    return None


# parse the html tag to return the body text <p class="tag">BODY</p>
def parse_body(elem: str):
    tag_end = cgu.enclose("<", ">", elem, 0)
    return elem[tag_end:]


# iteratively parse body to extract an element within a nested section
def parse_nested_body(elem: str, depth: int):
    iter = elem
    for i in range(0, depth):
        iter = parse_body(iter)
    end = iter.find("<")
    return iter[:end]


# function to sort release
def sort_func(kv):
    if kv != None:
        if "new_releases" in kv[1]:
            return kv[1]["new_releases"]
        else:
            now = datetime.datetime.now()
            return now.timestamp() - kv[1]["added"]


# parse entireity of a div section
def parse_div(html_str, div_class):
    divs = []
    while True:
        first = html_str.find(div_class)
        # no more left
        if first == -1:
            break
        start = html_str[:first].rfind("<div ")
        stack = 1
        iter_pos = first
        while stack > 0:
            open = cgu.us(html_str.find("<div", iter_pos))
            close = html_str.find("</div", iter_pos)
            if close != -1 and open < close:
                stack += 1
                iter_pos = open + 5
            elif close != -1:
                stack -= 1
                iter_pos = close + 5

        divs.append(html_str[start:iter_pos])

        # iterate
        html_str = html_str[iter_pos:]

    return divs


# parse entireity of a div section
def parse_class(html_str, html_class, ty):
    outputs = []
    while True:
        first = html_str.find(html_class)
        open_len = len("<{} ".format(ty))
        close_len = len("</{}>".format(ty))
        # no more left
        if first == -1:
            break
        start = html_str[:first].rfind("<{} ".format(ty))
        stack = 1
        iter_pos = first
        while stack > 0:
            open = cgu.us(html_str.find("<{}".format(ty), iter_pos))
            close = html_str.find("</{}".format(ty), iter_pos)
            if close != -1 and open < close:
                stack += 1
                iter_pos = open + open_len
            elif close != -1:
                stack -= 1
                iter_pos = close + close_len

        outputs.append(html_str[start:iter_pos])

        # iterate
        html_str = html_str[iter_pos:]

    return outputs


# testing
def firebase_test():
    # dig-19d4c
    # https://firestore.googleapis.com/v1/projects/diig/databases/(default)

    # Define the required scopes
    scopes = [
        "https://www.googleapis.com/auth/userinfo.email",
        "https://www.googleapis.com/auth/firebase.database"
    ]

    # Authenticate a credential with the service account
    credentials = service_account.Credentials.from_service_account_file(
        "diig-19d4c-firebase-adminsdk-jyja5-ebcf729661.json", scopes=scopes)

    # Use the credentials object to authenticate a Requests session.
    authed_session = AuthorizedSession(credentials)
    response = authed_session.get(
        "https://diig-19d4c-default-rtdb.europe-west1.firebasedatabase.app/")

    # get single
    response = authed_session.get(
        'https://diig-19d4c-default-rtdb.europe-west1.firebasedatabase.app/releases.json?orderBy="weekly_chart_deep-house"&startAt=0&endAt=100&print=pretty')

    print(response.text)
    pass


# write entire registry contents
def patch_releases(entries: str):
    scopes = [
        "https://www.googleapis.com/auth/userinfo.email",
        "https://www.googleapis.com/auth/firebase.database"
    ]

    credentials = service_account.Credentials.from_service_account_file(
        "diig-19d4c-firebase-adminsdk-jyja5-ebcf729661.json", scopes=scopes)

    authed_session = AuthorizedSession(credentials)
    response = authed_session.get(
        "https://diig-19d4c-default-rtdb.europe-west1.firebasedatabase.app/")
    assert(response.status_code == 200)

    response = authed_session.patch(
        "https://diig-19d4c-default-rtdb.europe-west1.firebasedatabase.app/releases.json", entries)
    assert(response.status_code == 200)


# scrape a store based on rules defined in stores.json config
def scrape_store(stores, store_name, page_function):
    if store_name in stores:
        # collapse store
        store = stores[store_name]
        # validate
        if "sections" not in store:
            print('error: "sections" missing from store config')
            exit(1)
        if "views" not in store:
            print('error: "views" missing from store config')
            exit(1)
        # iterate per section, per view
        for section in store["sections"]:
            for view in store["views"]:
                view_dict = store["views"][view]
                page_count = view_dict["page_count"]
                counter = 0
                print("scraping: {} / {}".format(section, view))
                for i in range(1, page_count):
                    view_url = view_dict["url"]
                    page_url = view_url.replace("${{section}}", section).replace("${{page}}", str(i))
                    counter = page_function(
                        page_url,
                        view,
                        section,
                        counter
                    )
    else:
        print("error: unknown store {}".format(store_name))
        exit(1)

# main
if __name__ == '__main__':
    # grab single flags
    get_urls = "-urls" in sys.argv
    test_single = "-test_single" in sys.argv
    verbose = "-verbose" in sys.argv

    # select store
    store = "redeye"
    if "-store" in sys.argv:
        store = sys.argv[sys.argv.index("-store") + 1]

    # stash service key for firebase writes
    if "-key" in sys.argv:
        key = sys.argv[sys.argv.index("-key") + 1]
        open("diig-19d4c-firebase-adminsdk-jyja5-ebcf729661.json", "w").write(key)

    # read store config
    stores = json.loads(open("stores.json", "r").read())

    # parse individual stores
    if store == "juno":
        scrape_store(stores, "juno", juno.scrape_page)
    elif store == "redeye":
        redeye.scrape_legacy(100, get_urls, test_single, verbose)
    elif store == "redeye2":
        redeye.scrape(100, get_urls, verbose)
    elif store == "test":
        firebase_test()