function dynareParallelRmDir(PRCDir,Parallel)
% PARALLEL CONTEXT
% In a parallel context, this is a specialized version of rmdir() function.
%
% INPUTS
%  o PRCDir         []   ... 
%  o Parallel       []   ...  
%
%  OUTPUTS
%  None
%
%
%
% Copyright (C) 2009-2010 Dynare Team
%
% This file is part of Dynare.
%
% Dynare is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% Dynare is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with Dynare.  If not, see <http://www.gnu.org/licenses/>.



if nargin ==0,
    disp('dynareParallelRmDir(fname)')
    return
end

for indPC=1:length(Parallel),
    while (1)
        if isunix
            stat = system(['ssh ',Parallel(indPC).UserName,'@',Parallel(indPC).ComputerName,' rm -fr ',Parallel(indPC).RemoteDirectory,'/',PRCDir]);
            break;
        else
            [stat, mess, id] = rmdir(['\\',Parallel(indPC).ComputerName,'\',Parallel(indPC).RemoteDrive,'$\',Parallel(indPC).RemoteDirectory,'\',PRCDir],'s');
            if stat==1,
                break,
            else
                if isempty(dynareParallelDir(PRCDir,'',Parallel));
                    break,
                else
                    pause(1);
                end
            end
        end
    end
end

return