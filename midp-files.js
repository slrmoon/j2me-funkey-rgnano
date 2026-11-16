
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
    loadPackage({"files": [{"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 0, "end": 1900}, {"filename": "/midp/lib/screen.image_bg.png", "start": 1900, "end": 2048}, {"filename": "/midp/lib/title.image_bg1.png", "start": 2048, "end": 2229}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 2229, "end": 2539}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 2539, "end": 2702}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 2702, "end": 3438}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 3438, "end": 3661}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 3661, "end": 5940}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 5940, "end": 7313}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 7313, "end": 7533}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 7533, "end": 7867}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 7867, "end": 8027}, {"filename": "/midp/lib/scroll.image_up.png", "start": 8027, "end": 8265}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 8265, "end": 8472}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 8472, "end": 8649}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 8649, "end": 10036}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 10036, "end": 10769}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 10769, "end": 10989}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 10989, "end": 11142}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 11142, "end": 11396}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 11396, "end": 11549}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 11549, "end": 14190}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 14190, "end": 14413}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 14413, "end": 14630}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 14630, "end": 16717}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 16717, "end": 17666}, {"filename": "/midp/lib/untrusted_icon.png", "start": 17666, "end": 18695}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 18695, "end": 23576}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 23576, "end": 23946}, {"filename": "/midp/lib/title.image_bg2.png", "start": 23946, "end": 24546}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 24546, "end": 26711}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 26711, "end": 28684}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 28684, "end": 28895}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 28895, "end": 29073}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 29073, "end": 30810}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 30810, "end": 31146}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 31146, "end": 32469}, {"filename": "/midp/lib/pbar.image_values.png", "start": 32469, "end": 34362}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 34362, "end": 35443}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 35443, "end": 36170}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 36170, "end": 37022}, {"filename": "/midp/lib/title.image_bg0.png", "start": 37022, "end": 37865}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 37865, "end": 38159}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 38159, "end": 38360}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 38360, "end": 40578}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 40578, "end": 40810}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 40810, "end": 40996}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 40996, "end": 42981}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 42981, "end": 43141}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 43141, "end": 44935}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 44935, "end": 45229}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 45229, "end": 46940}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 46940, "end": 47110}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 47110, "end": 47270}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 47270, "end": 47489}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 47489, "end": 48414}, {"filename": "/midp/lib/doja-support.jar", "start": 48414, "end": 930858}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 930858, "end": 931047}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 931047, "end": 931360}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 931360, "end": 931571}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 931571, "end": 931860}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 931860, "end": 932050}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 932050, "end": 932947}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 932947, "end": 933136}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 933136, "end": 933301}, {"filename": "/midp/lib/indicator_network.png", "start": 933301, "end": 933821}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 933821, "end": 933981}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 933981, "end": 934317}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 934317, "end": 934477}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 934477, "end": 934709}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 934709, "end": 934884}, {"filename": "/midp/lib/indicator_home.png", "start": 934884, "end": 935493}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 935493, "end": 937054}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 937054, "end": 937257}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 937257, "end": 937444}, {"filename": "/midp/lib/trusted_icon.png", "start": 937444, "end": 938236}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 938236, "end": 938443}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 938443, "end": 938656}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 938656, "end": 938842}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 938842, "end": 939143}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 939143, "end": 941100}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 941100, "end": 942536}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 942536, "end": 942837}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 942837, "end": 943606}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 943606, "end": 943829}, {"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 943829, "end": 943979}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 943979, "end": 944149}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 944149, "end": 944635}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 944635, "end": 944788}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 944788, "end": 945737}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 945737, "end": 947227}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 947227, "end": 947516}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 947516, "end": 948355}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 948355, "end": 948518}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 948518, "end": 950301}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 950301, "end": 951297}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 951297, "end": 954941}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 954941, "end": 955112}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 955112, "end": 955313}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 955313, "end": 955469}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 955469, "end": 955638}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 955638, "end": 955951}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 955951, "end": 956125}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 956125, "end": 956676}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 956676, "end": 956864}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 956864, "end": 957096}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 957096, "end": 957328}, {"filename": "/midp/lib/calendar.png", "start": 957328, "end": 957459}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 957459, "end": 957943}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 957943, "end": 958162}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 958162, "end": 959917}, {"filename": "/midp/lib/indicator_bar.png", "start": 959917, "end": 961862}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 961862, "end": 964392}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 964392, "end": 964559}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 964559, "end": 964814}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 964814, "end": 964973}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 964973, "end": 967268}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 967268, "end": 968021}, {"filename": "/midp/lib/gauge.image_values.png", "start": 968021, "end": 969160}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 969160, "end": 969325}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 969325, "end": 971020}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 971020, "end": 971207}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 971207, "end": 973582}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 973582, "end": 975494}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 975494, "end": 976166}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 976166, "end": 976393}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 976393, "end": 977584}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 977584, "end": 978497}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 978497, "end": 980279}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 980279, "end": 982008}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 982008, "end": 984335}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 984335, "end": 984558}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 984558, "end": 984744}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 984744, "end": 985453}, {"filename": "/midp/lib/skin.bin", "start": 985453, "end": 991229}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 991229, "end": 991396}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 991396, "end": 992691}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 992691, "end": 992928}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 992928, "end": 993307}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 993307, "end": 993557}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 993557, "end": 993758}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 993758, "end": 995530}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 995530, "end": 997434}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 997434, "end": 997854}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 997854, "end": 999638}, {"filename": "/midp/lib/indicator_trusted.png", "start": 999638, "end": 1000485}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 1000485, "end": 1001152}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 1001152, "end": 1001384}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 1001384, "end": 1002387}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 1002387, "end": 1002561}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 1002561, "end": 1004345}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 1004345, "end": 1006089}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 1006089, "end": 1006357}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 1006357, "end": 1007030}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 1007030, "end": 1008541}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 1008541, "end": 1009360}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 1009360, "end": 1009567}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 1009567, "end": 1009864}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 1009864, "end": 1010824}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 1010824, "end": 1011001}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 1011001, "end": 1011166}, {"filename": "/midp/lib/screen.image_wash.png", "start": 1011166, "end": 1011328}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 1011328, "end": 1011501}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 1011501, "end": 1011733}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 1011733, "end": 1011894}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 1011894, "end": 1012075}, {"filename": "/midp/appdb/_suite8.png", "start": 1012075, "end": 1012867}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 1012867, "end": 1013587}, {"filename": "/midp/appdb/_single8.png", "start": 1013587, "end": 1014385}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 1014385, "end": 1017721}, {"filename": "/midp/appdb/_main.ks", "start": 1017721, "end": 1018694}, {"filename": "/midp/appdb/_dukeok8.png", "start": 1018694, "end": 1022718}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 1022718, "end": 1031443}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 1031443, "end": 1034407}, {"filename": "/midp/appdb/_ch_suite.png", "start": 1034407, "end": 1037709}, {"filename": "/midp/appdb/_ch_single.png", "start": 1037709, "end": 1040924}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 1040924, "end": 1049688}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1049688, "end": 1050753}, {"filename": "/midp/appdb/_ack8.png", "start": 1050753, "end": 1054486}], "remote_package_size": 1054486, "package_uuid": "acb33306-71df-4c0b-a73d-2e1d420d35f5"});

  })();
