
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
    loadPackage({"files": [{"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 0, "end": 1900}, {"filename": "/midp/lib/screen.image_bg.png", "start": 1900, "end": 2048}, {"filename": "/midp/lib/title.image_bg1.png", "start": 2048, "end": 2229}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 2229, "end": 2539}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 2539, "end": 2702}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 2702, "end": 3438}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 3438, "end": 3661}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 3661, "end": 5940}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 5940, "end": 7313}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 7313, "end": 7533}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 7533, "end": 7867}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 7867, "end": 8027}, {"filename": "/midp/lib/scroll.image_up.png", "start": 8027, "end": 8265}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 8265, "end": 8472}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 8472, "end": 8649}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 8649, "end": 10036}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 10036, "end": 10769}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 10769, "end": 10989}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 10989, "end": 11142}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 11142, "end": 11396}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 11396, "end": 11549}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 11549, "end": 14190}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 14190, "end": 14413}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 14413, "end": 14630}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 14630, "end": 16717}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 16717, "end": 17666}, {"filename": "/midp/lib/untrusted_icon.png", "start": 17666, "end": 18695}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 18695, "end": 23576}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 23576, "end": 23946}, {"filename": "/midp/lib/title.image_bg2.png", "start": 23946, "end": 24546}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 24546, "end": 26711}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 26711, "end": 28684}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 28684, "end": 28895}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 28895, "end": 29073}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 29073, "end": 30810}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 30810, "end": 31146}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 31146, "end": 32469}, {"filename": "/midp/lib/pbar.image_values.png", "start": 32469, "end": 34362}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 34362, "end": 35443}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 35443, "end": 36170}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 36170, "end": 37022}, {"filename": "/midp/lib/title.image_bg0.png", "start": 37022, "end": 37865}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 37865, "end": 38159}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 38159, "end": 38360}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 38360, "end": 40578}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 40578, "end": 40810}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 40810, "end": 40996}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 40996, "end": 42981}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 42981, "end": 43141}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 43141, "end": 44935}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 44935, "end": 45229}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 45229, "end": 46940}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 46940, "end": 47110}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 47110, "end": 47270}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 47270, "end": 47489}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 47489, "end": 48414}, {"filename": "/midp/lib/doja-support.jar", "start": 48414, "end": 931331}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 931331, "end": 931520}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 931520, "end": 931833}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 931833, "end": 932044}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 932044, "end": 932333}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 932333, "end": 932523}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 932523, "end": 933420}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 933420, "end": 933609}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 933609, "end": 933774}, {"filename": "/midp/lib/indicator_network.png", "start": 933774, "end": 934294}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 934294, "end": 934454}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 934454, "end": 934790}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 934790, "end": 934950}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 934950, "end": 935182}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 935182, "end": 935357}, {"filename": "/midp/lib/indicator_home.png", "start": 935357, "end": 935966}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 935966, "end": 937527}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 937527, "end": 937730}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 937730, "end": 937917}, {"filename": "/midp/lib/trusted_icon.png", "start": 937917, "end": 938709}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 938709, "end": 938916}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 938916, "end": 939129}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 939129, "end": 939315}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 939315, "end": 939616}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 939616, "end": 941573}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 941573, "end": 943009}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 943009, "end": 943310}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 943310, "end": 944079}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 944079, "end": 944302}, {"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 944302, "end": 944452}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 944452, "end": 944622}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 944622, "end": 945108}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 945108, "end": 945261}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 945261, "end": 946210}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 946210, "end": 947700}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 947700, "end": 947989}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 947989, "end": 948828}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 948828, "end": 948991}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 948991, "end": 950774}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 950774, "end": 951770}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 951770, "end": 955414}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 955414, "end": 955585}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 955585, "end": 955786}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 955786, "end": 955942}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 955942, "end": 956111}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 956111, "end": 956424}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 956424, "end": 956598}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 956598, "end": 957149}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 957149, "end": 957337}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 957337, "end": 957569}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 957569, "end": 957801}, {"filename": "/midp/lib/calendar.png", "start": 957801, "end": 957932}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 957932, "end": 958416}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 958416, "end": 958635}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 958635, "end": 960390}, {"filename": "/midp/lib/indicator_bar.png", "start": 960390, "end": 962335}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 962335, "end": 964865}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 964865, "end": 965032}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 965032, "end": 965287}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 965287, "end": 965446}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 965446, "end": 967741}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 967741, "end": 968494}, {"filename": "/midp/lib/gauge.image_values.png", "start": 968494, "end": 969633}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 969633, "end": 969798}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 969798, "end": 971493}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 971493, "end": 971680}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 971680, "end": 974055}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 974055, "end": 975967}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 975967, "end": 976639}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 976639, "end": 976866}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 976866, "end": 978057}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 978057, "end": 978970}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 978970, "end": 980752}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 980752, "end": 982481}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 982481, "end": 984808}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 984808, "end": 985031}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 985031, "end": 985217}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 985217, "end": 985926}, {"filename": "/midp/lib/skin.bin", "start": 985926, "end": 991702}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 991702, "end": 991869}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 991869, "end": 993164}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 993164, "end": 993401}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 993401, "end": 993780}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 993780, "end": 994030}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 994030, "end": 994231}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 994231, "end": 996003}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 996003, "end": 997907}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 997907, "end": 998327}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 998327, "end": 1000111}, {"filename": "/midp/lib/indicator_trusted.png", "start": 1000111, "end": 1000958}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 1000958, "end": 1001625}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 1001625, "end": 1001857}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 1001857, "end": 1002860}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 1002860, "end": 1003034}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 1003034, "end": 1004818}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 1004818, "end": 1006562}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 1006562, "end": 1006830}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 1006830, "end": 1007503}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 1007503, "end": 1009014}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 1009014, "end": 1009833}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 1009833, "end": 1010040}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 1010040, "end": 1010337}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 1010337, "end": 1011297}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 1011297, "end": 1011474}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 1011474, "end": 1011639}, {"filename": "/midp/lib/screen.image_wash.png", "start": 1011639, "end": 1011801}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 1011801, "end": 1011974}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 1011974, "end": 1012206}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 1012206, "end": 1012367}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 1012367, "end": 1012548}, {"filename": "/midp/appdb/_suite8.png", "start": 1012548, "end": 1013340}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 1013340, "end": 1014060}, {"filename": "/midp/appdb/_single8.png", "start": 1014060, "end": 1014858}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 1014858, "end": 1018194}, {"filename": "/midp/appdb/_main.ks", "start": 1018194, "end": 1019167}, {"filename": "/midp/appdb/_dukeok8.png", "start": 1019167, "end": 1023191}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 1023191, "end": 1031916}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 1031916, "end": 1034880}, {"filename": "/midp/appdb/_ch_suite.png", "start": 1034880, "end": 1038182}, {"filename": "/midp/appdb/_ch_single.png", "start": 1038182, "end": 1041397}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 1041397, "end": 1050161}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1050161, "end": 1051226}, {"filename": "/midp/appdb/_ack8.png", "start": 1051226, "end": 1054959}], "remote_package_size": 1054959, "package_uuid": "cb857555-047d-4bf3-b28a-00d228932513"});

  })();
