# Dig

Dig is a record digging app that provides a high performance audio player and ergonomic user interface to make digging through record snippits rapid, responsive and enjoyable. It provides a record store agnostic audio player with a familiar social media infinite scrolling interface to allow users to dig for new releases and provide click through links to stores to buy. Feeds and snippits are cached which also enables offline browsing.

<img src="https://github.com/polymonster/dig/raw/main/media/dig.gif" align=”center”/>

## Schemas

The project provides schemas and a unified data respresentation so that other record stores and sources can be added to expand the database.

### Release Schema

A release schema defines a single release, specifying artist, track names, track urls of snippits and artworks amogst other information. Release are stored in a flat dictionay with the key being their id plus the store name prefix to avoid ID collisions, each release is treated uniquely per record store. In the future aggregating releases and cross checking them might be a feature worth looking into.

```json
{
    "id": "string - store specific id for the release",
    "link": "string - url link to the store specific page for the release",
    "artist": "string - the artist name",
    "title": "string - the releases title name",
    "label": "string - name of the label",
    "label_link": "string - url link to the store specific page for the label",
    "cat": "string - the catalog number for the label of the release",
    "artworks": [
        "string list - 3 artwork sizes: [small, medium, large]"
    ],
    "track_names": [
        "string list - track names"
    ],
    "track_urls": [
        "string list - url links to track mp3s"
    ],
    "store": "string - the specifc store this release is from",
    "store_tags": {
        "out_of_stock": true,
        "has_sold_out": true,
        "has_charted": true,
        "preorder": true
    },

    "<store>-<section>-<view>": "int - position in a view for particular store and section",
    "Example Genre": "Genre Tag"
}
```

### Store Schema

A store can specify parts of a record store website to track. Sections define genre groups, Views define charts and latest releases. You can supply the special variables `${{section}}` and `${{page}}` to record store urls

```json
{
    "sections": [
        "string list - Sections names to parse from the website, these strings will appear in the website urls",
    ],
    "section_display_names": [
        "string list - Display names for the sections for rendering within the app"
    ],
    "views": {
        "weekly_chart": {
            "url": "https://www.recordstore.co.uk/${{section}}/weekly-chart",
            "page_count": "int or null - number of pages to parse for the url"
        },
        "monthly_chart": {
            "url": "https://www.recordstore.co.uk/${{section}}/monthly-chart",
            "page_count": null
        },
        "new_releases": {
            "url": "https://www.recordstore.co.uk/${{section}}/new-releases/page-${{page}}",
            "page_count": 100
        }
    }
}
```

## Scrapers

Scrapers run [nightly](https://github.com/polymonster/dig/actions) to update information, if you provide a store schema then you need to define a page scraper function. There are currently a few examples in the project. It involves parsing the release schema information from the stores html pages.

## Firebase

Data is uploaded and synchronised to firebase with data publicly readable. For a dump of the entire releases database you can use:

```text
curl https://diig-19d4c-default-rtdb.europe-west1.firebasedatabase.app/releases.json
```
