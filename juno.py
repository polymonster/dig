import urllib.request
import cgu
import json
import sys
import os
import datetime
import dig

# parse ide element <div id="item-861163-1" class="dv-item"> to extract the
# id number without the item- prefix
def parse_id(id_elem: str):
    item_str = dig.get_value(id_elem, "id")
    id = item_str.strip("item-")
    return id


# parse link url out of <a href="/products/moy-on-wire-breaking-the-loop-ep-vinyl/861163-01/">
# and concatonate to https://www.juno.co.uk
def parse_link(link_elem: str):
    link = dig.get_value(link_elem, "href")
    return 'https://www.juno.co.uk' + link


# extract label body from a long html string, it is the body element at the very end
def parse_label(label_elem: str):
    start = label_elem.rfind(">")
    return label_elem[start+1:]


# extract the catalogue number of a release from <div class="vi-text mb-1">Cat: UFOS 004. Rel:&nbsp;28 Mar 22
def parse_cat(cat_elem: str):
    info_str = dig.parse_body(cat_elem)
    cat_end = info_str.rfind(".")
    cat_start = info_str.rfind("Cat:") + len("Cat:")
    return info_str[cat_start:cat_end].strip()


# parse small artwork url from <img class="lazy_img img-fluid" src="https://imagescdn.juno.co.uk/150/CS869223-01A.jpg" alt="420">
# construct them as so:
# small - https://imagescdn.juno.co.uk/full/CS861163-01A-BIG.jpg
# med - https://imagescdn.juno.co.uk/300/CS983218-01A-MED.jpg
# large - https://imagescdn.juno.co.uk/150/CS861163-01A.jpg
def parse_artworks(artwork_elem):
    small = dig.get_value(artwork_elem, "src")
    if small.find("data:image") != -1:
        small = dig.get_value(artwork_elem, "data-src")
    med = small.replace("/150/", "/300/").replace(".jpg", "-MED.jpg")
    large = small.replace("/150/", "/full/").replace(".jpg", "-BIG.jpg")
    return [small, med, large]


# parse track urls and names out of list of html elems
# <div class="listing-track">
#   <div><a class="btn btn-play-sm" href="https://www.juno.co.uk/MP3/SF869223-01-02-03.mp3" data-ua_action="play" rel="nofollow" data-turbo="false"></a></div>
# <div>Untitled 5 (4:19)</div>
# return as tuple (track_names, track_urls)
def parse_tracks(tracks):
    track_names = []
    track_urls = []
    for track in tracks:
        url = dig.get_value(track, "href")
        nest = 6
        if url is None:
            nest = 4
        else:
            track_urls.append(url)
        name = dig.parse_nested_body(track, nest)
        track_names.append(name)
    return (track_names, track_urls)


# main entry for juno scrape
def scrape(page_count):
    # weekly chart
    for i in range(1, 5):
        scrape_page("https://www.juno.co.uk/minimal-tech-house/charts/bestsellers/this-week/{}".format(i), "weekly_chart")

    # monthly chart
    for i in range(1, 5):
        scrape_page("https://www.juno.co.uk/minimal-tech-house/charts/bestsellers/four-weeks/{}".format(i), "monthtly_chart")

    # latest
    counter = 0
    for i in range(1, page_count):
        counter = scrape_page("https://www.juno.co.uk/minimal-tech-house/eight-weeks/{}/?order=date_down".format(i), "latest", counter)
        if counter == None:
            break


# scape a single page with counter tracking
def scrape_page(url, category, counter = 0):
    print("scraping: juno ", url, flush=True)

    # try and then continue if the page does not exist
    try:
        html_file = urllib.request.urlopen(url)
    except:
        return

    html_str = html_file.read().decode("utf8")

    # store release entries in dict
    releases_dict = dict()

    # gets products
    products = dig.parse_div(html_str, 'class="product-list"')

    # separate into items
    releases = dig.parse_div(products[0], 'class="dv-item"')

    # open existing reg
    reg_filepath = "registry/juno.json"
    if os.path.exists(reg_filepath):
        releases_dict = json.loads(open(reg_filepath, "r").read())

    for release in releases:
        # parse divs and sections of htmls
        (_, id_elem) = dig.find_parse_elem(release, 0, "<div id=", ">")
        (_, link_elem) = dig.find_parse_elem(release, 0, "<a href=", ">")
        (_, artwork_elem) = dig.find_parse_elem(release, 0, "<img class", ">")

        (offset, artist_elem) = dig.find_parse_elem(release, 0, '<a class="text-md"', "</a>")
        (offset, title_elem) = dig.find_parse_elem(release, offset, '<a class="text-md"', "</a>")
        (offset, label_elem) = dig.find_parse_elem(release, offset, '<a class="text-md"', "</a>")
        (offset, cat_elem) = dig.find_parse_elem(release, offset, '<div class="vi-text', "<br class")

        # tracks
        tracks = dig.parse_div(release, 'class="listing-track')

        # extract values from the html sections
        release_dict = dict()
        release_dict["store"] = "juno"
        release_dict["id"] = parse_id(id_elem)
        release_dict["link"] = parse_link(link_elem)
        release_dict["artist"] = dig.parse_body(artist_elem)
        release_dict["title"] = dig.parse_body(title_elem)
        release_dict["label"] = parse_label(label_elem)
        release_dict["cat"] = parse_cat(cat_elem)
        release_dict["artworks"] = parse_artworks(artwork_elem)
        (release_dict["track_names"], release_dict["track_urls"]) = parse_tracks(tracks)

        # chart pos
        if category in ["weekly_chart", "monthly_chart"]:
            pos = dig.parse_nested_body(release, 4).strip()
            release_dict[category] = pos
        else:
            release_dict[category] = counter

        # increment counter to track latest indices
        counter += 1

        releases_dict[release_dict["id"]] = release_dict

        # merge into main
        merge = dict()
        merge[release_dict["id"]] = release_dict
        dig.merge_dicts(releases_dict, merge)

    # write to file
    release_registry = (json.dumps(releases_dict, indent=4))
    open(reg_filepath, "w+").write(release_registry)

    return counter