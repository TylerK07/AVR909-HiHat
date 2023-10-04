const FS = require('fs');
const crypto = require('crypto');
const Jimp = require('jimp');



// Light weight binary file reading class
function binStream( data ){
  this.data = data;
  this.ptr  = 0;
  this.readChar = () => {
    let chr = data.readUInt8( this.ptr++ )
    return( chr != 0 ? String.fromCharCode( chr ) : "" );
  }
  this.readString = (length) => {
    let str = "";
    for( let i=0; i<length; i++ ){ str += this.readChar(); }
    return( str );
  }

  this.readUInt8    = () => data.readUInt8( this.ptr++ );
  this.readUInt16BE = () => { this.ptr+=2; return( data.readUInt16BE(this.ptr-2) ); }
  this.readUInt16LE = () => { this.ptr+=2; return( data.readUInt16LE(this.ptr-2) ); }
  this.readUInt32BE = () => { this.ptr+=4; return( data.readUInt32BE(this.ptr-4) ); }
  this.readUInt32LE = () => { this.ptr+=4; return( data.readUInt32LE(this.ptr-4) ); }
  this.readInt8     = () => data.readInt8( this.ptr++ );
  this.readInt16BE  = () => { this.ptr+=2; return( data.readInt16BE(this.ptr-2) );  }
  this.readInt16LE  = () => { this.ptr+=2; return( data.readInt16LE(this.ptr-2) );  }
  this.readInt32BE  = () => { this.ptr+=4; return( data.readInt32BE(this.ptr-4) );  }
  this.readInt32LE  = () => { this.ptr+=4; return( data.readInt32LE(this.ptr-4) );  }
  this.readDoubleBE = () => { this.ptr+=8; return( data.readDoubleBE(this.ptr-8) ); }
  this.readDoubleLE = () => { this.ptr+=8; return( data.readDoubleLE(this.ptr-8) ); }
}

// Convert a value into a 2-digit hex value with a leading zero if necessary
function toHex( v ){
  let val = v.toString(16).toUpperCase();
  if( val.length < 2 ) val = "0"+val;
  return( val )
}

function processWAVFile( fileName ){
  let fileData = new binStream( FS.readFileSync( `./${fileName}`) );
  
  let headerInfo = {
	  c0_id:              fileData.readString(4),
	  c0_size:            fileData.readInt32LE(),
	  c0_format:          fileData.readString(4),
	  c1_id:              fileData.readString(4),
	  c1_size:            fileData.readInt32LE(),
	  c1_audio_format:    fileData.readInt16LE(),
	  c1_num_channels:    fileData.readInt16LE(),
	  c1_sample_rate:     fileData.readInt32LE(),
	  c1_byte_rate:       fileData.readInt32LE(),
	  c1_block_align:     fileData.readInt16LE(),
	  c1_bits_per_sample: fileData.readInt16LE(),
	  c2_id:              fileData.readString(4),
	  c2_size:            fileData.readInt32LE()
  };
  console.log( headerInfo );
  
  console.log( `Generating 6-bit wav format for ${headerInfo.c2_size} bytes...`)
  
  // To keep things simple, we are going to assume that the file is byts is divisible by 4
  
  if( headerInfo.c2_size % 4 > 0 ){
  	console.log( "data size needs to be divisible by 4");
  	return;
  } else {
  	console.log( `data size is divisible into ${headerInfo.c2_size / 4} 4-byte chunks!` );
  }

  let numChunks = headerInfo.c2_size / 4;

  var outStr = "";
  

  let image = new Jimp(headerInfo.c2_size, 64 );
	let x = 0;
	for( let i = 0; i < numChunks; i++ ){

		// Read the next 4 bytes
		let b1 = fileData.readUInt8();// & 0b11111100;
		let b2 = fileData.readUInt8();// & 0b11111100;
		let b3 = fileData.readUInt8();// & 0b11111100;
		let b4 = fileData.readUInt8();// & 0b11111100;

		// Plot each of the bytes onto the waveform image
		image.setPixelColor(Jimp.rgbaToInt(0x00, 0xFF, 0xFF, 0xFF), ++x, b1>>>2);
		image.setPixelColor(Jimp.rgbaToInt(0x00, 0xFF, 0xFF, 0xFF), ++x, b2>>>2);
		image.setPixelColor(Jimp.rgbaToInt(0x00, 0xFF, 0xFF, 0xFF), ++x, b3>>>2);
		image.setPixelColor(Jimp.rgbaToInt(0x00, 0xFF, 0xFF, 0xFF), ++x, b4>>>2);


	  // Compress the 4 bytes into 3 by stripping out the lowest 2 bits of each byte
		let o1 = (  b1     | (b2>>6) ) & 0xFF;
		let o2 = ( (b2<<2) | (b3>>4) ) & 0xFF;
		let o3 = ( (b3<<4) | (b4>>2) ) & 0xFF;



	  // Write the values of each byte in "0xHH," hex format to an output string
		outStr += `0x${toHex(o1)},`;  	
		outStr += `0x${toHex(o2)},`;  	
		outStr += `0x${toHex(o3)},`;  	

	}

	// Write out the image data to a png file
	image.write('originalWav.png', (err) => { if (err) throw err; });

	// Strip off the last comma
	outStr = outStr.substr(0, outStr.length-1);

	// Generate a debug.js file that is formatted in javascript so we can include it in visualizeWav.js
  FS.writeFileSync("debug.js", `sample = [${outStr}];`);

	// Format the output string to be a .h header file that can be included in our program
  outStr = `#ifndef SAMPLE_H\n#define SAMPLE_H\nconst unsigned char sample[${numChunks*3}] PROGMEM = {${outStr}}\n#endif`;
  return( outStr );
}


let inputFile = ""
let outputFile = "wav_c.h"
if (process.argv[2]) {
  inputFile = process.argv[2];
  if (process.argv[3]) {
    outputFile = process.argv[3];
  } else {
    outputFile = inputFile.split(".")[0] + ".h"
  }

  FS.writeFileSync(outputFile, processWAVFile(inputFile));
  console.log( "Done!");
	
} else {
  console.log('\n');
  console.log('------------------------------------------------');
  console.log(' WAV FILE PARSER                                ');
  console.log('------------------------------------------------');
  console.log('\n');


  console.log('Please specify an input file using syntax: \n');
  console.log('node op2.js file.wav \n');
}