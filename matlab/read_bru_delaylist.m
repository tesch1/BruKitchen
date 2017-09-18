function delays = read_bru_delaylist(filename)
%
% read a bruker-style delay list, convert u and m to usec and msec values
% (c) 2016, michael tesch
%

% matlab is stupid, this expands tildes in filenames:
blah = fopen(filename);
filename = fopen(blah);
fclose(blah);

A = textread(filename,'%s','whitespace','\n');

% tetermine the kind of line
TypeOfRow = cell(length(A),2);

R = {   ...
    '^([\d\.]+) *u$', 'us' ; ...
    '^([\d\.]+) *m$', 'ms' ; ...
    '^([\d\.]+) *s$', 's' ; ...
    '^([\d\.]+) *$' , 's' ; ...
    '^$'      , 'Empty'    ...
    };

for i=1:length(A)
    for j=1:size(R,1)
        [s,t] = regexp(A{i},R{j,1},'start','tokens');
        if (~isempty(s))
            TypeOfRow{i,1}=R{j,2};
            TypeOfRow{i,2}=t{1};
            break;
        end
    end
    if isempty(s)
        A
        error(sprintf('unformatted line %d in file %s\n', i, filename));
    end
end

jj = 1;
for ii=1:length(TypeOfRow)
    switch TypeOfRow{ii,1}
      case 'us'
        delays(jj) = 1e-6 * str2num(TypeOfRow{ii,2}{1});
      case 'ms'
        delays(jj) = 1e-3 * str2num(TypeOfRow{ii,2}{1});
      case 's'
        delays(jj) = str2num(TypeOfRow{ii,2}{1});
    end
    %disp(sprintf('%s / %s -> %f\n', TypeOfRow{ii,1}, TypeOfRow{ii,2}{1}, delays(jj)));
    jj = jj + 1;
end

delays = delays(:);
