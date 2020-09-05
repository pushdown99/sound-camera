var fs          = require('fs');
var path        = require('path');
var express     = require('express');
var fileUpload  = require('express-fileupload');
var router      = express.Router();
var app         = express();
var moment      = require('moment-timezone');
var port        = process.env.PORT || 9900;

app.set('views', __dirname + '/views');
app.set('view engine', 'ejs');
app.engine('html', require('ejs').renderFile);

app.use(fileUpload());
app.use(express.json());
app.use(express.urlencoded({ extended: true}));
app.use(express.static(__dirname + '/public'));

/////////////////////////////////////////////////////////////////////////
//
// middleware
//
app.use(function (req, res, next) {
    req.timestamp  = moment().unix();
    req.receivedAt = moment().tz('Asia/Seoul').format('YYYY-MM-DD HH:mm:ss');
    console.log(req.receivedAt + ': ', req.method, req.protocol +'://' + req.hostname + req.url);
  
    return next();
  });

//////////////////////////////////////////////////////////////
//
// Express routing
//
app.route('/')
  .get(function(req, res, next){
    res.send('get');
  });

app.route('/upload')
  .get(function(req, res, next){
    res.render('upload');
  });

app.route('/noise')
  .get(function(req, res, next){
    const directoryPath = path.join(__dirname, "/public/data/")
    console.log(directoryPath)
    var images = new Array();
    fs.readdir(directoryPath, function(err, files) {
        if (err) {
          console.log("Error getting directory information.")
        } else {
          files.forEach(function(file) {
            console.log(file)
            var data = new Object() ;
            data.name = file;
            images.push(data);
          })
        }
        var json = JSON.stringify(images) ;
        console.log(json);
        res.render('noise', {'json': images});
    })
  });

app.post('/upload', function(req, res) {
    console.log(req);
    if (!req.files || Object.keys(req.files).length === 0) {
        return res.status(400).send('No files were uploaded.');
    }
    let sampleFile = req.files.sampleFile;
    console.log(sampleFile);
  
    sampleFile.mv(path.resolve(__dirname, './public/data', moment().tz('Asia/Seoul').format('YYYYMMDD-HHmmss') + '.jpg'), function(err) {
      if (err)
        return res.status(500).send(err);
  
      res.send('File uploaded!');
    });
});

////////////////////////////////////////////////////////
// listener
app.listen(port, function(){
    console.log('Listener: ', 'Example app listening on port ' + port);
});

module.exports = app;
