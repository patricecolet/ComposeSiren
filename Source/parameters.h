//
//  parameters.h
//  ComposeSirenes
//
//  Created by Gauthier Segay on 12/12/2021.
//  Copyright © 2021 pollux louette. All rights reserved.
//

#ifndef parameters_h
#define parameters_h


enum MidiControlChangeParameters {
    ResetAllController = 121
};

namespace Commands {

    const unsigned char Reset[] = {CMD_RESET,0X04,0X00,0X00};

}

#endif /* parameters_h */
