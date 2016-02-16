% RTDSP LAB 4
% Mattin Mir-Tahmasebi & Ahmed Ibrahim
% Imperial College London 2016 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%  firpmord  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Setting the frequencies for the stop/passband cutoffs
freqStop1 = 375;
freqPass1 = 450;
freqPass2 = 1600;
freqStop2 = 1750;

% Specufying the desired amplitudes at the stopbands/passband
% The ampStop values are the amplitudes until the first cutoff and after
% the last cutoff, respectively. ampPass is the amplitude between the 
% 2nd and 3rd cutoff. Between the 1st/2nd and 3rd/4th cutoffs are 
% transition periods where the amplitude is changing so it doesnt make
% sense to specify an amplitude.
ampStop1 = 0;
ampPass = 1;
ampStop2 = 0;

% Specifying the deired amplitude deviation at the stopbands/passband
% Same explanation as aboe for why there are 3 values.
devStop_dB = -48;
devPass_dB = 0.4;

% Convering dB to absolute values
devStop1 = 10^(devStop_dB/20);
devPass = (10^(devPass_dB/20)-1)/(10^(devPass_dB/20)+1);
devStop2 = 10^(devStop_dB/20);

% Sampling frequency
fs = 8000;

% Cell array to hold output of firpmord
% n : filter order
% fo : frequency vector
% ao : amplitude response vector
% w : weights
[n, fo, ao, w] = firpmord([freqStop1 freqPass1 freqPass2 freqStop2], ...
        [ampStop1 ampPass ampStop2], [devStop1 devPass devStop2], fs);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% firpm %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% row vector of n+1 coefficients of the order n FIR filter whose 
% frequency-amplitude characteristics match those given by vectors f and a
order = n+4;
b = firpm(order, fo, ao, w);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Output %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Save coefficients to a text file 'fir_coefs.txt'
format long e;

formatSpec = '%2.15e';
coefs = num2str(b',formatSpec);
coefs = cellstr(coefs);
data = strjoin(coefs', ', ');

fileID = fopen('H:\RTDSPlab\lab4\RTDSP\fir_coef.txt', 'w');
fprintf(fileID, 'double b[]={');
fprintf(fileID,data);
fprintf (fileID, '};');
fclose(fileID);

% Plotting freq response
freqz(b,1,1024,fs);
