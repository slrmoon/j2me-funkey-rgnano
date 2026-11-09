
  var Module = typeof Module !== 'undefined' ? Module : {};

  if (!Module.expectedDataFileDownloads) {
    Module.expectedDataFileDownloads = 0;
  }

  Module.expectedDataFileDownloads++;
  (function() {
    // When running as a pthread, FS operations are proxied to the main thread, so we don't need to
    // fetch the .data bundle on the worker
    if (Module['ENVIRONMENT_IS_PTHREAD']) return;
    var loadPackage = function(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.toString().substring(0, window.location.pathname.toString().lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.toString().substring(0, location.pathname.toString().lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = '/home/claude/j2me-funkey-rgnano/out/web/site/midp-files.data';
      var REMOTE_PACKAGE_BASE = 'midp-files.data';
      if (typeof Module['locateFilePackage'] === 'function' && !Module['locateFile']) {
        Module['locateFile'] = Module['locateFilePackage'];
        err('warning: you defined Module.locateFilePackage, that has been renamed to Module.locateFile (using your locateFilePackage for now)');
      }
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;

      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];
      var PACKAGE_UUID = metadata['package_uuid'];

      function fetchRemotePackage(packageName, packageSize, callback, errback) {
        if (typeof process === 'object' && typeof process.versions === 'object' && typeof process.versions.node === 'string') {
          require('fs').readFile(packageName, function(err, contents) {
            if (err) {
              errback(err);
            } else {
              callback(contents.buffer);
            }
          });
          return;
        }
        var xhr = new XMLHttpRequest();
        xhr.open('GET', packageName, true);
        xhr.responseType = 'arraybuffer';
        xhr.onprogress = function(event) {
          var url = packageName;
          var size = packageSize;
          if (event.total) size = event.total;
          if (event.loaded) {
            if (!xhr.addedTotal) {
              xhr.addedTotal = true;
              if (!Module.dataFileDownloads) Module.dataFileDownloads = {};
              Module.dataFileDownloads[url] = {
                loaded: event.loaded,
                total: size
              };
            } else {
              Module.dataFileDownloads[url].loaded = event.loaded;
            }
            var total = 0;
            var loaded = 0;
            var num = 0;
            for (var download in Module.dataFileDownloads) {
            var data = Module.dataFileDownloads[download];
              total += data.total;
              loaded += data.loaded;
              num++;
            }
            total = Math.ceil(total * Module.expectedDataFileDownloads/num);
            if (Module['setStatus']) Module['setStatus']('Downloading data... (' + loaded + '/' + total + ')');
          } else if (!Module.dataFileDownloads) {
            if (Module['setStatus']) Module['setStatus']('Downloading data...');
          }
        };
        xhr.onerror = function(event) {
          throw new Error("NetworkError for: " + packageName);
        }
        xhr.onload = function(event) {
          if (xhr.status == 200 || xhr.status == 304 || xhr.status == 206 || (xhr.status == 0 && xhr.response)) { // file URLs can return 0
            var packageData = xhr.response;
            callback(packageData);
          } else {
            throw new Error(xhr.statusText + " : " + xhr.responseURL);
          }
        };
        xhr.send(null);
      };

      function handleError(error) {
        console.error('package error:', error);
      };

      var fetchedCallback = null;
      var fetched = Module['getPreloadedPackage'] ? Module['getPreloadedPackage'](REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE) : null;

      if (!fetched) fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE, function(data) {
        if (fetchedCallback) {
          fetchedCallback(data);
          fetchedCallback = null;
        } else {
          fetched = data;
        }
      }, handleError);

    function runWithFS() {

      function assert(check, msg) {
        if (!check) throw msg + new Error().stack;
      }
Module['FS_createPath']("/", "midp", true, true);
Module['FS_createPath']("/midp", "lib", true, true);
Module['FS_createPath']("/midp", "appdb", true, true);

      /** @constructor */
      function DataRequest(start, end, audio) {
        this.start = start;
        this.end = end;
        this.audio = audio;
      }
      DataRequest.prototype = {
        requests: {},
        open: function(mode, name) {
          this.name = name;
          this.requests[name] = this;
          Module['addRunDependency']('fp ' + this.name);
        },
        send: function() {},
        onload: function() {
          var byteArray = this.byteArray.subarray(this.start, this.end);
          this.finish(byteArray);
        },
        finish: function(byteArray) {
          var that = this;
          // canOwn this data in the filesystem, it is a slide into the heap that will never change
          Module['FS_createDataFile'](this.name, null, byteArray, true, true, true);
          Module['removeRunDependency']('fp ' + that.name);
          this.requests[this.name] = null;
        }
      };

      var files = metadata['files'];
      for (var i = 0; i < files.length; ++i) {
        new DataRequest(files[i]['start'], files[i]['end'], files[i]['audio'] || 0).open('GET', files[i]['filename']);
      }

      function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer instanceof ArrayBuffer, 'bad input to processPackageData');
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          DataRequest.prototype.byteArray = byteArray;
          var files = metadata['files'];
          for (var i = 0; i < files.length; ++i) {
            DataRequest.prototype.requests[files[i].filename].onload();
          }          Module['removeRunDependency']('datafile_/home/claude/j2me-funkey-rgnano/out/web/site/midp-files.data');

      };
      Module['addRunDependency']('datafile_/home/claude/j2me-funkey-rgnano/out/web/site/midp-files.data');

      if (!Module.preloadResults) Module.preloadResults = {};

      Module.preloadResults[PACKAGE_NAME] = {fromCache: false};
      if (fetched) {
        processPackageData(fetched);
        fetched = null;
      } else {
        fetchedCallback = processPackageData;
      }

    }
    if (Module['calledRun']) {
      runWithFS();
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module["preRun"].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 0, "end": 150}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 150, "end": 1341}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 1341, "end": 1515}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 1515, "end": 1716}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 1716, "end": 1876}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 1876, "end": 2099}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 2099, "end": 2302}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 2302, "end": 2615}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 2615, "end": 2838}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 2838, "end": 3007}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 3007, "end": 4443}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 4443, "end": 4603}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 4603, "end": 4778}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 4778, "end": 5262}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 5262, "end": 5934}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 5934, "end": 6166}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 6166, "end": 7553}, {"filename": "/midp/lib/pbar.image_values.png", "start": 7553, "end": 9446}, {"filename": "/midp/lib/indicator_network.png", "start": 9446, "end": 9966}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 9966, "end": 10173}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 10173, "end": 10467}, {"filename": "/midp/lib/skin.bin", "start": 10467, "end": 16243}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 16243, "end": 16544}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 16544, "end": 17540}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 17540, "end": 17837}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 17837, "end": 18000}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 18000, "end": 18165}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 18165, "end": 18397}, {"filename": "/midp/lib/indicator_bar.png", "start": 18397, "end": 20342}, {"filename": "/midp/lib/gauge.image_values.png", "start": 20342, "end": 21481}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 21481, "end": 21698}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 21698, "end": 23427}, {"filename": "/midp/lib/calendar.png", "start": 23427, "end": 23558}, {"filename": "/midp/lib/title.image_bg0.png", "start": 23558, "end": 24401}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 24401, "end": 25891}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 25891, "end": 26624}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 26624, "end": 27549}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 27549, "end": 29534}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 29534, "end": 29694}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 29694, "end": 30180}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 30180, "end": 30999}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 30999, "end": 31249}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 31249, "end": 31435}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 31435, "end": 31667}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 31667, "end": 34308}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 34308, "end": 34602}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 34602, "end": 35329}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 35329, "end": 35561}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 35561, "end": 35829}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 35829, "end": 36199}, {"filename": "/midp/lib/trusted_icon.png", "start": 36199, "end": 36991}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 36991, "end": 37151}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 37151, "end": 37860}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 37860, "end": 39155}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 39155, "end": 40866}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 40866, "end": 43241}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 43241, "end": 43575}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 43575, "end": 43763}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 43763, "end": 44615}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 44615, "end": 44928}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 44928, "end": 45348}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 45348, "end": 47627}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 47627, "end": 47828}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 47828, "end": 48048}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 48048, "end": 48267}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 48267, "end": 49106}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 49106, "end": 51636}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 51636, "end": 51925}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 51925, "end": 53006}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 53006, "end": 53162}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 53162, "end": 54111}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 54111, "end": 54288}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 54288, "end": 54453}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 54453, "end": 54664}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 54664, "end": 56401}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 56401, "end": 58619}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 58619, "end": 58826}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 58826, "end": 58979}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 58979, "end": 59715}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 59715, "end": 61498}, {"filename": "/midp/lib/indicator_home.png", "start": 61498, "end": 62107}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 62107, "end": 62339}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 62339, "end": 62593}, {"filename": "/midp/lib/indicator_trusted.png", "start": 62593, "end": 63440}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 63440, "end": 63991}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 63991, "end": 65895}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 65895, "end": 66118}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 66118, "end": 66331}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 66331, "end": 66490}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 66490, "end": 66668}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 66668, "end": 66887}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 66887, "end": 67088}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 67088, "end": 67299}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 67299, "end": 68052}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 68052, "end": 70379}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 70379, "end": 72352}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 72352, "end": 72519}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 72519, "end": 72672}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 72672, "end": 72973}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 72973, "end": 73352}, {"filename": "/midp/lib/untrusted_icon.png", "start": 73352, "end": 74381}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 74381, "end": 75384}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 75384, "end": 76153}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 76153, "end": 77714}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 77714, "end": 77874}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 77874, "end": 78060}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 78060, "end": 79815}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 79815, "end": 81902}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 81902, "end": 82799}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 82799, "end": 82966}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 82966, "end": 83255}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 83255, "end": 84950}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 84950, "end": 85170}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 85170, "end": 87465}, {"filename": "/midp/lib/title.image_bg1.png", "start": 87465, "end": 87646}, {"filename": "/midp/lib/screen.image_bg.png", "start": 87646, "end": 87794}, {"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 87794, "end": 89694}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 89694, "end": 94575}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 94575, "end": 96357}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 96357, "end": 96530}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 96530, "end": 98487}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 98487, "end": 100271}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 100271, "end": 100503}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 100503, "end": 102415}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 102415, "end": 102670}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 102670, "end": 104043}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 104043, "end": 105837}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 105837, "end": 106026}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 106026, "end": 106249}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 106249, "end": 107162}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 107162, "end": 107498}, {"filename": "/midp/lib/screen.image_wash.png", "start": 107498, "end": 107660}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 107660, "end": 108327}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 108327, "end": 111971}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 111971, "end": 112141}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 112141, "end": 112312}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 112312, "end": 113272}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 113272, "end": 113449}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 113449, "end": 113602}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 113602, "end": 115767}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 115767, "end": 115953}, {"filename": "/midp/lib/scroll.image_up.png", "start": 115953, "end": 116191}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 116191, "end": 116418}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 116418, "end": 116599}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 116599, "end": 116788}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 116788, "end": 116949}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 116949, "end": 117114}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 117114, "end": 118437}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 118437, "end": 118607}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 118607, "end": 118943}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 118943, "end": 119616}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 119616, "end": 121388}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 121388, "end": 123172}, {"filename": "/midp/lib/title.image_bg2.png", "start": 123172, "end": 123772}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 123772, "end": 123979}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 123979, "end": 124289}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 124289, "end": 124526}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 124526, "end": 126037}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 126037, "end": 126224}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 126224, "end": 126387}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 126387, "end": 128131}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 128131, "end": 128305}, {"filename": "/midp/lib/doja-support.jar", "start": 128305, "end": 983426}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 983426, "end": 983613}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 983613, "end": 983803}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 983803, "end": 984752}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 984752, "end": 987716}, {"filename": "/midp/appdb/_single8.png", "start": 987716, "end": 988514}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 988514, "end": 989234}, {"filename": "/midp/appdb/_ch_suite.png", "start": 989234, "end": 992536}, {"filename": "/midp/appdb/_main.ks", "start": 992536, "end": 993509}, {"filename": "/midp/appdb/_suite8.png", "start": 993509, "end": 994301}, {"filename": "/midp/appdb/_dukeok8.png", "start": 994301, "end": 998325}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 998325, "end": 1007050}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 1007050, "end": 1010386}, {"filename": "/midp/appdb/_ch_single.png", "start": 1010386, "end": 1013601}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1013601, "end": 1014666}, {"filename": "/midp/appdb/_ack8.png", "start": 1014666, "end": 1018399}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 1018399, "end": 1027163}], "remote_package_size": 1027163, "package_uuid": "f89f7654-2c31-4bed-921d-34a4281b5de1"});

  })();
