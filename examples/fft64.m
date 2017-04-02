function y = fft64(x)
if size(x) ~= [64 1]
    error('fft size is not [64 1]')
else
	for ii=0:31
		s1(ii+1:32:64,1) = fft2(x(ii+1:32:64,1));
	end
for jj=0:15
	for ii=0:1
		s2(jj+ii*16+1:32:64,1) = fft2(exp(-j*pi*(0:1)'*ii/2).*s1(jj+ii*32+1:16:jj+ii*32+32,1));
	end
end
for jj=0:7
	for ii=0:3
		s3(jj+ii*8+1:32:64,1) = fft2(exp(-j*pi*(0:1)'*ii/4).*s2(jj+ii*16+1:8:jj+ii*16+16,1));
	end
end
for jj=0:3
	for ii=0:7
		s4(jj+ii*4+1:32:64,1) = fft2(exp(-j*pi*(0:1)'*ii/8).*s3(jj+ii*8+1:4:jj+ii*8+8,1));
	end
end
for jj=0:1
    for ii=0:15
        s5(jj+ii*2+1:32:64,1) = fft2(exp(-j*pi*(0:1)'*ii/16).*s4(jj+ii*4+1:2:jj+ii*4+4,1));
    end
end
    for ii=0:31
        s6(ii+1:32:64,1) = fft2(exp(-j*pi*(0:1)'*ii/32).*s5(ii*2+1:1:ii*2+2,1));
    end
    y = s6;
end
