
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
      var PACKAGE_NAME = '/home/runner/work/j2me-funkey-rgnano/j2me-funkey-rgnano/out/web/site/midp-files.data';
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
          }          Module['removeRunDependency']('datafile_/home/runner/work/j2me-funkey-rgnano/j2me-funkey-rgnano/out/web/site/midp-files.data');

      };
      Module['addRunDependency']('datafile_/home/runner/work/j2me-funkey-rgnano/j2me-funkey-rgnano/out/web/site/midp-files.data');

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
    loadPackage({"files": [{"filename": "/midp/lib/menu.image_bg7.png", "start": 0, "end": 268}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 268, "end": 500}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 500, "end": 754}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 754, "end": 1750}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 1750, "end": 1939}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 1939, "end": 3912}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 3912, "end": 4119}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 4119, "end": 6494}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 6494, "end": 7045}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 7045, "end": 7232}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 7232, "end": 7459}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 7459, "end": 8384}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 8384, "end": 8870}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 8870, "end": 9873}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 9873, "end": 11246}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 11246, "end": 11409}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 11409, "end": 12845}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 12845, "end": 13056}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 13056, "end": 15143}, {"filename": "/midp/lib/trusted_icon.png", "start": 15143, "end": 15935}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 15935, "end": 17630}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 17630, "end": 17793}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 17793, "end": 19697}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 19697, "end": 21862}, {"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 21862, "end": 22012}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 22012, "end": 22219}, {"filename": "/midp/lib/indicator_home.png", "start": 22219, "end": 22828}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 22828, "end": 22998}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 22998, "end": 27879}, {"filename": "/midp/lib/title.image_bg2.png", "start": 27879, "end": 28479}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 28479, "end": 30190}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 30190, "end": 30413}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 30413, "end": 31122}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 31122, "end": 31323}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 31323, "end": 31540}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 31540, "end": 31726}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 31726, "end": 31886}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 31886, "end": 32265}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 32265, "end": 32554}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 32554, "end": 32786}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 32786, "end": 33453}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 33453, "end": 33742}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 33742, "end": 34823}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 34823, "end": 35783}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 35783, "end": 35944}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 35944, "end": 38585}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 38585, "end": 38955}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 38955, "end": 39249}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 39249, "end": 39481}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 39481, "end": 41225}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 41225, "end": 41412}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 41412, "end": 43324}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 43324, "end": 43510}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 43510, "end": 43730}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 43730, "end": 45524}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 45524, "end": 45821}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 45821, "end": 45988}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 45988, "end": 46289}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 46289, "end": 46442}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 46442, "end": 46632}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 46632, "end": 46966}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 46966, "end": 47143}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 47143, "end": 47563}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 47563, "end": 47899}, {"filename": "/midp/lib/pbar.image_values.png", "start": 47899, "end": 49792}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 49792, "end": 49993}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 49993, "end": 50942}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 50942, "end": 51109}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 51109, "end": 51269}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 51269, "end": 51506}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 51506, "end": 52259}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 52259, "end": 52482}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 52482, "end": 54761}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 54761, "end": 55674}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 55674, "end": 56401}, {"filename": "/midp/lib/title.image_bg1.png", "start": 56401, "end": 56582}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 56582, "end": 58143}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 58143, "end": 58456}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 58456, "end": 59843}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 59843, "end": 61572}, {"filename": "/midp/lib/scroll.image_up.png", "start": 61572, "end": 61810}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 61810, "end": 62707}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 62707, "end": 62872}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 62872, "end": 63032}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 63032, "end": 63209}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 63209, "end": 63362}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 63362, "end": 64098}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 64098, "end": 66083}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 66083, "end": 66315}, {"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 66315, "end": 68215}, {"filename": "/midp/lib/screen.image_wash.png", "start": 68215, "end": 68377}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 68377, "end": 68566}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 68566, "end": 68879}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 68879, "end": 69134}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 69134, "end": 69290}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 69290, "end": 69513}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 69513, "end": 69814}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 69814, "end": 70021}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 70021, "end": 70315}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 70315, "end": 71048}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 71048, "end": 71226}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 71226, "end": 71407}, {"filename": "/midp/lib/gauge.image_values.png", "start": 71407, "end": 72546}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 72546, "end": 72699}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 72699, "end": 72931}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 72931, "end": 73132}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 73132, "end": 73302}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 73302, "end": 75084}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 75084, "end": 75297}, {"filename": "/midp/lib/untrusted_icon.png", "start": 75297, "end": 76326}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 76326, "end": 78110}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 78110, "end": 78329}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 78329, "end": 78639}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 78639, "end": 78862}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 78862, "end": 79033}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 79033, "end": 79221}, {"filename": "/midp/lib/indicator_bar.png", "start": 79221, "end": 81166}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 81166, "end": 81331}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 81331, "end": 82522}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 82522, "end": 84033}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 84033, "end": 84802}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 84802, "end": 86759}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 86759, "end": 88514}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 88514, "end": 88674}, {"filename": "/midp/lib/indicator_network.png", "start": 88674, "end": 89194}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 89194, "end": 90143}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 90143, "end": 90393}, {"filename": "/midp/lib/doja-support.jar", "start": 90393, "end": 945514}, {"filename": "/midp/lib/indicator_trusted.png", "start": 945514, "end": 946361}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 946361, "end": 946547}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 946547, "end": 948765}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 948765, "end": 948934}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 948934, "end": 949606}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 949606, "end": 949809}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 949809, "end": 950661}, {"filename": "/midp/lib/skin.bin", "start": 950661, "end": 956437}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 956437, "end": 958732}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 958732, "end": 960515}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 960515, "end": 961188}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 961188, "end": 961347}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 961347, "end": 961521}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 961521, "end": 961741}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 961741, "end": 961973}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 961973, "end": 962147}, {"filename": "/midp/lib/calendar.png", "start": 962147, "end": 962278}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 962278, "end": 964808}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 964808, "end": 965027}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 965027, "end": 967354}, {"filename": "/midp/lib/screen.image_bg.png", "start": 967354, "end": 967502}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 967502, "end": 969239}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 969239, "end": 969575}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 969575, "end": 969786}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 969786, "end": 970625}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 970625, "end": 971948}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 971948, "end": 975592}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 975592, "end": 977376}, {"filename": "/midp/lib/title.image_bg0.png", "start": 977376, "end": 978219}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 978219, "end": 979709}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 979709, "end": 979884}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 979884, "end": 981179}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 981179, "end": 981352}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 981352, "end": 981512}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 981512, "end": 981677}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 981677, "end": 982161}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 982161, "end": 983933}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 983933, "end": 984752}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 984752, "end": 993516}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 993516, "end": 994236}, {"filename": "/midp/appdb/_dukeok8.png", "start": 994236, "end": 998260}, {"filename": "/midp/appdb/_main.ks", "start": 998260, "end": 999233}, {"filename": "/midp/appdb/_suite8.png", "start": 999233, "end": 1000025}, {"filename": "/midp/appdb/_single8.png", "start": 1000025, "end": 1000823}, {"filename": "/midp/appdb/_ch_suite.png", "start": 1000823, "end": 1004125}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 1004125, "end": 1007089}, {"filename": "/midp/appdb/_ack8.png", "start": 1007089, "end": 1010822}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 1010822, "end": 1019547}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1019547, "end": 1020612}, {"filename": "/midp/appdb/_ch_single.png", "start": 1020612, "end": 1023827}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 1023827, "end": 1027163}], "remote_package_size": 1027163, "package_uuid": "2733507b-7e3c-40e0-884c-c4cb9b7414cd"});

  })();
