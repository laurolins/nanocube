var view_schemata = {

    "flights": { 
        url: "http://rcloud-lba-02.research.att.com/nanocube/13",
        title: "Flights",
        max_zoom: 25,
        time_range: [ new Date(1987,0,1), new Date(2009, 0, 1) ],
        center: { lat: 39.82, lon: -83.57, zoom: 2 },
        views: [
            { type: "count" },
            { type: "histogram",
              height: 170,
              field: { 
                  name: "ontime",
                  title: "On Time",
                  values: ["61+_min_early",
                           "31_60_min_early", 
                           "16_30_min_early", 
                           "6_15_min_early", 
                           "5_min_earlylate", 
                           "6_15_min_late", 
                           "16_30_min_late", 
                           "31_60_min_late", 
                           "61+_min_late"]
              }
            },
            { type: "histogram",
              height: 120,
              field: {
                  name: "carrier",
                  title: "Carrier",
                  values: ["Delta","Southwest","American","US_Air","United",
                           "Northwest","Continental"
                           //"Midway","Pacific_Southwest","Aloha","ATA","Hawaiian","Pan_Am",
                           // "Frontier","Pinnacle","Independence",
                           // "JetBlue",
                           //"Mesa","Piedmont","Eastern","AirTran","Comair",
                           //"Altantic_Southest","Expressjet",
                           // "Alaska","Skywest",
                           // "America_West","TWA","American_Eagle"
                          ]
              }
            },
            { type: "time-series",
              height: 100
            }
        ],
        parent_div: "#vis-panes"
    },

    "brightkite": {
        url: "http://rcloud-lba-02.research.att.com/nanocube/14",
        title: "Brightkite Checkins",
        center: { lat: 0, lon: 0, zoom: 1 },
        time_range: [ new Date(2008,0,1), new Date(2010, 11, 1) ],
        max_zoom: 25,
        views: [
            { type: "count" },
            { type: "histogram",
              height: 100,
              border: 0,
              field: { 
                  name: "dayofweek",
                  title: "Day of Week",
                  values: ["Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "Sun"]
              }
            },
            { type: "histogram", 
              height: 320,
              border: 0,
              field: {
                  name: "hour",
                  title: "Hour",
                  values: ["0", "1", "2", "3", "4", "5",
                           "6", "7", "8", "9", "10", "11", 
                           "12", "13", "14", "15", "16", "17",
                           "18", "19", "20", "21", "22", "23"]
              }
            },
            { type: "binned-scatterplot",
              border: 0,
              height: 100,
              field_x: {
                  name: "hour",
                  title: "Hour",
                  values: ["0", "1", "2", "3", "4", "5",
                           "6", "7", "8", "9", "10", "11", 
                           "12", "13", "14", "15", "16", "17",
                           "18", "19", "20", "21", "22", "23"]
              },
              field_y: { 
                  name: "dayofweek",
                  title: "Day of Week",
                  values: ["Sun", "Sat", "Fri", "Thr", "Wed", "Tue", "Mon"]
              }
            },
            { type: "time-series",
              height: 80
            }
        ],
        parent_div: "#vis-panes"
    },

    "gowalla": {
        url: "http://rcloud-lba-02.research.att.com/nanocube/12",
        title: "Gowalla Checkins",
        center: { lat: 0, lon: 0, zoom: 1 },
        time_range: [ new Date(2008,0,1), new Date(2010, 11, 1) ],
        max_zoom: 20,
        views: [
            { type: "count" },
            { type: "histogram",
              height: 100,
              border: 0,
              field: { 
                  name: "dayofweek",
                  title: "Day of Week",
                  values: ["Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "Sun"]
              }
            },
            { type: "histogram", 
              height: 320,
              border: 0,
              field: {
                  name: "hour",
                  title: "Hour",
                  values: ["0", "1", "2", "3", "4", "5",
                           "6", "7", "8", "9", "10", "11", 
                           "12", "13", "14", "15", "16", "17",
                           "18", "19", "20", "21", "22", "23"]
              }
            },
            { type: "binned-scatterplot",
              border: 0,
              height: 100,
              field_x: {
                  name: "hour",
                  title: "Hour",
                  values: ["0", "1", "2", "3", "4", "5",
                           "6", "7", "8", "9", "10", "11", 
                           "12", "13", "14", "15", "16", "17",
                           "18", "19", "20", "21", "22", "23"]
              },
              field_y: { 
                  name: "dayofweek",
                  title: "Day of Week",
                  values: ["Sun", "Sat", "Fri", "Thr", "Wed", "Tue", "Mon"]
              }
            },
            { type: "time-series",
              height: 80
            }
        ],
        parent_div: "#vis-panes"
    },

    "twitter": {
        url: "http://rcloud-lba-02.research.att.com/nanocube/15",
        title: "Geo-located Tweets",
        center: { lat: 37.82, lon: -88.57, zoom: 10 },
        time_range: [ new Date(2011, 10, 1), new Date(2012, 10, 1) ],
        max_zoom: 15,
        views: [
            { type: "count" },
            { type: "histogram", 
              height: 80,
              field: {
                  name: "device",
                  values: ["none", "iphone", "android", "ipad", "windows"]
              }
            },
            { type: "time-series",
              height: 150
            }
        ],
        parent_div: "#vis-panes"
    },

    "twitter_seattle": {
        url: "http://rcloud-lba-02.research.att.com/nanocube/16",
        title: "Geo-located Tweets in Seattle",
        center: { lat: 47.5, lon: -122.3, zoom: 500 },
        time_range: [ new Date(2011, 10, 1), new Date(2012, 10, 1) ],
        max_zoom: 25,
        views: [
            { type: "count" },
            { type: "histogram", 
              height: 80,
              field: {
                  name: "device",
                  values: ["none", "iphone", "android", "ipad", "windows"]
              }
            },
            { type: "time-series",
              height: 150
            }
        ],
        parent_div: "#vis-panes"
    },

    "twitter_chicago": {
        url: "http://rcloud-lba-02.research.att.com/nanocube/17",
        title: "Geo-located Tweets in Chicago",
        center: { lat: 41.8, lon: -87.7, zoom: 500 },
        time_range: [ new Date(2011, 10, 1), new Date(2012, 10, 1) ],
        max_zoom: 25,
        views: [
            { type: "count" },
            { type: "histogram", 
              height: 80,
              field: {
                  name: "device",
                  values: ["none", "iphone", "android", "ipad", "windows"]
              }
            },
            { type: "time-series",
              height: 150
            }
        ],
        parent_div: "#vis-panes"
    },

    "twitter_new_york": {
        url: "http://rcloud-lba-02.research.att.com/nanocube/18",
        title: "Geo-located Tweets in New York",
        center: { lat: 40.766, lon: -74, zoom: 500 },
        time_range: [ new Date(2011, 10, 1), new Date(2012, 10, 1) ],
        max_zoom: 25,
        views: [
            { type: "count" },
            { type: "histogram", 
              height: 80,
              field: {
                  name: "device",
                  values: ["none", "iphone", "android", "ipad", "windows"]
              }
            },
            { type: "time-series",
              height: 150
            }
        ],
        parent_div: "#vis-panes"
    }
};
