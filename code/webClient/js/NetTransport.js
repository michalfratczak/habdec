

function DecodeSpectrum(i_buffer, i_offset)
{
	var header =
	{
		header_size_ : 0,	// int32_t
		noise_floor_ : 0,	// float
		noise_variance_ : 0,	// float
		sampling_rate_: 0, // float
		shift_ : 0,	// float
		peak_left_ : 0,	// int32_t
		peak_right_ : 0,	// int32_t
		peak_left_valid_ : 0,	// int32_t
		peak_right_valid_ : 0,	// int32_t
		min_ : 0,	// float
		max_ : 0,	// float

		type_size_ : 0,	// int32_t. value:s 1/2/4 for uint8 uint16 float32
		size_ : 0	// int32_t
	};

	var dv = new DataView(i_buffer, i_offset);

	var offset = 0;
	header.header_size_ = 		dv.getInt32		(offset, true); 	offset += 4;
	header.noise_floor_ = 		dv.getFloat32	(offset, true); 	offset += 4;
	header.noise_variance_ =	dv.getFloat32	(offset, true); 	offset += 4;
	header.sampling_rate_=		dv.getFloat32	(offset, true); 	offset += 4;
	header.shift_  =			dv.getFloat32	(offset, true); 	offset += 4;
	header.peak_left_  = 		dv.getInt32		(offset, true); 	offset += 4;
	header.peak_right_  = 		dv.getInt32		(offset, true); 	offset += 4;
	header.peak_left_valid_=	dv.getInt32		(offset, true); 	offset += 4;
	header.peak_right_valid_=	dv.getInt32		(offset, true); 	offset += 4;
	header.min_  = 			 	dv.getFloat32	(offset, true); 	offset += 4;
	header.max_  = 			 	dv.getFloat32	(offset, true); 	offset += 4;
	header.type_size_ =			dv.getInt32		(offset, true); 	offset += 4;
	header.size_ = 				dv.getInt32		(offset, true); 	offset += 4;

	// add data
	if(header.type_size_ == 1) // 8 bit char
		header.values_ = new Uint8Array( i_buffer, offset, header.size_);
	else if(header.type_size_ == 2) // uint16_t
		header.values_ = new Uint16Array( i_buffer, offset, header.size_);
	else if(header.type_size_ == 4) // 32float
		header.values_ = new Float32Array( i_buffer, offset, header.size_);

	return header;
}

function DecodeDemod(i_buffer, i_offset)
{
	var header =
	{
		header_size_ : 0,	// int32_t
		min_ : 0,	// float
		max_ : 0,	// float
		type_size_ : 0,	// int32_t. value:s 1/2/4 for uint8 uint16 float32
		size_ : 0	// int32_t
	};

	var dv = new DataView(i_buffer, i_offset);

	var offset = 0;
	header.header_size_ = 		dv.getInt32		(offset, true); 	offset += 4;
	header.min_  = 			 	dv.getFloat32	(offset, true); 	offset += 4;
	header.max_  = 			 	dv.getFloat32	(offset, true); 	offset += 4;
	header.type_size_ =			dv.getInt32		(offset, true); 	offset += 4;
	header.size_ = 				dv.getInt32		(offset, true); 	offset += 4;

	// add data
	if(header.type_size_ == 1) // 8 bit char
		header.values_ = new Uint8Array( i_buffer, offset, header.size_);
	else if(header.type_size_ == 2) // uint16_t
		header.values_ = new Uint16Array( i_buffer, offset, header.size_);
	else if(header.type_size_ == 4) // 32float
		header.values_ = new Float32Array( i_buffer, offset, header.size_);

	return header;
}
